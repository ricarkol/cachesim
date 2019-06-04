#include "htable.h"
#include "lru.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "list.h"
#define MALLOC(x)	malloc(x)
#define FREE(x)		free(x)

lru_page *lru_new_page(uint64_t n, uint64_t data)
{
	lru_page *p;
	p = MALLOC(sizeof(lru_page));
	p->n = n;
	p->dirty = 0;
	p->data = data;
	return p;
}

ht_key_t lru_get_key(void *a)
{
	return (ht_key_t) ((lru_page *) a)->n;
}

lru_t *lru_init(uint64_t cc)
{
	lru_t *lru;
	lru = MALLOC(sizeof(lru_t));
	if (!lru)
		return NULL;
	lru->c = cc;
	lru->len = 0;
	lru->io = 0;
	lru->hits = 0;
	lru->evictions = 0;
	lru->ht = ht_init(10, lru_get_key);
	if (!lru->ht) {
		FREE(lru);
		return NULL;
	}
	dl_list_init(&lru->links);
	return lru;
}

void lru_free(lru_t *lru)
{
	ht_free(lru->ht);
	FREE(lru);
}

/*
 * Returns a page if hit
 */
lru_page *lru_search(lru_t *lru, uint64_t n)
{
	lru_page *p;

	lru->io++;
	p = ht_search(lru->ht, n);
	if (p) {	/* Hit */
		dl_list_del(&p->links);
		dl_list_add_tail(&lru->links, &p->links);
		lru->hits++;
	}
	return p;
}

// evicted page should be freed by caller
void lru_add(lru_t *lru, lru_page *new, lru_page **evicted)
{
	*evicted = NULL;
	if (lru->c == 0) {
		return;
	}

	if (lru->len == lru->c) {	/* evict */
		*evicted = dl_list_first(&lru->links, lru_page, links);
		dl_list_del(&((*evicted)->links));
		ht_remove(lru->ht, (*evicted));
		if ((*evicted)->dirty) /* dirty evictions */
			lru->evictions++;
		lru->len--;
	}
	dl_list_add_tail(&lru->links, &new->links);
	ht_add(lru->ht, new->n, new);
	lru->len++;
}

/* size change */
void lru_inc_size(lru_t *lru, uint64_t inc)
{
	lru->c += inc;
}

void lru_dec_size(lru_t *lru, lru_page **evicted)
{
        *evicted = NULL;
	if (lru->c == 0) {
		return;
	}

	if (lru->len == lru->c) {	/* evict */
		*evicted = dl_list_first(&lru->links, lru_page, links);
		dl_list_del(&((*evicted)->links));
		ht_remove(lru->ht, (*evicted));
		if ((*evicted)->dirty) /* dirty evictions */
			lru->evictions++;
		lru->len--;
	}
	lru->c--;
}
