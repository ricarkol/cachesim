#include "htable.h"
#include "mru.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "list.h"
#define MALLOC(x)	malloc(x)
#define FREE(x)		free(x)

mru_page *mru_new_page(uint64_t n, uint64_t data)
{
	mru_page *p;
	p = MALLOC(sizeof(mru_page));
	p->n = n;
	p->dirty = 0;
	p->data = data;
	return p;
}

ht_key_t mru_get_key(void *a)
{
	return (ht_key_t) ((mru_page *) a)->n;
}

mru_t *mru_init(uint64_t cc)
{
	mru_t *mru;
	mru = MALLOC(sizeof(mru_t));
	if (!mru)
		return NULL;
	mru->c = cc;
	mru->len = 0;
	mru->io = 0;
	mru->evictions = 0;
	mru->ht = ht_init(20, mru_get_key);
	if (!mru->ht) {
		FREE(mru);
		return NULL;
	}
	dl_list_init(&mru->links);
	return mru;
}

void mru_free(mru_t *mru)
{
	ht_free(mru->ht);
	FREE(mru);
}

/*
 * Returns a page if hit
 */
mru_page *mru_search(mru_t *mru, uint64_t n)
{
	mru_page *p;

	mru->io++;
	p = ht_search(mru->ht, n);
	if (p) {	/* Hit */
		dl_list_del(&p->links);
		dl_list_add_tail(&mru->links, &p->links);
		mru->hits++;
	}
	return p;
}

// evicted page should be freed by caller
void mru_add(mru_t *mru, mru_page *new, mru_page **evicted)
{
	*evicted = NULL;
	if (mru->c == 0) {
		return;
	}

	if (mru->len == mru->c) {	/* evict */
		*evicted = dl_list_last(&mru->links, mru_page, links);
		dl_list_del(&(*evicted)->links);
		ht_remove(mru->ht, (*evicted));
		if ((*evicted)->dirty) /* dirty evictions */
			mru->evictions++;
		mru->len--;
	}
	dl_list_add_tail(&mru->links, &new->links);
	ht_add(mru->ht, new->n, new);
	mru->len++;
}


/* size change */
void mru_inc_size(mru_t *mru, uint64_t inc)
{
	mru->c += inc;
}

void mru_dec_size(mru_t *mru, mru_page **evicted)
{
	*evicted = NULL;
	if (mru->c == 0) {
		return;
	}

	if (mru->len == mru->c) {	/* evict */
		*evicted = dl_list_last(&mru->links, mru_page, links);
		dl_list_del(&(*evicted)->links);
		ht_remove(mru->ht, (*evicted));
		if ((*evicted)->dirty) /* dirty evictions */
			mru->evictions++;
		mru->len--;
	}
	mru->c--;
}
