#include "htable.h"
#include "arc.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "list.h"
#define MALLOC(x)	malloc(x)
#define FREE(x)		free(x)
#define _ASSERT(x)	assert(x)

#ifndef MAX
#define MAX(a,b)	((a>b) ? (a):(b))
#endif
#ifndef MIN
#define MIN(a,b)	((a<b) ? (a):(b))
#endif


ht_key_t arc_get_key(void *a)
{
	return (ht_key_t) ((arc_page *) a)->n;
}

arc_page *arc_new_page(uint64_t n, uint64_t data)
{
	arc_page *p;
	p = MALLOC(sizeof(arc_page));
	p->n = n;
	p->where = -1;
	p->dirty = 0;
	p->data = data;
	return p;
}

#define ARC_INIT_LIST(ll)					\
		arc->len[ll] = 0;				\
		dl_list_init(&arc->links[ll]);

arc_t *arc_init(uint64_t cc)
{
	arc_t *arc;
	arc = MALLOC(sizeof(arc_t));
	if (!arc)
		return NULL;
	arc->C = cc;
	arc->T = 0;
	arc->hits = 0;
	arc->io = 0;
	arc->evictions = 0;
	arc->ht = ht_init(20, arc_get_key);
	if (!arc->ht) {
		FREE(arc);
		return NULL;
	}
	ARC_INIT_LIST(_T1_);
	ARC_INIT_LIST(_T2_);
	ARC_INIT_LIST(_B1_);
	ARC_INIT_LIST(_B2_);
	return arc;
}

int arc_free(arc_t *arc)
{
	int i;
	ht_free(arc->ht); // XXX memory leak who frees the lists?
	FREE(arc);
}

void arc_list_remove(arc_t *arc, arc_page *p)
{
	dl_list_del(&p->links);
	arc->len[p->where]--;
	p->where = -1;
}

void arc_list_add(arc_t *arc, arc_page *p, arc_list dest)
{
	dl_list_add_tail(&arc->links[dest], &p->links);
	p->where = dest;
	arc->len[dest]++;
}

arc_page *arc_remove_lru(arc_t *arc, arc_list from)
{
	assert(arc->len[from] > 0);
	arc_page *p = dl_list_first(&arc->links[from], arc_page, links);
	arc_list_remove(arc, p);
	return p;
}

arc_page *arc_replace(arc_t *arc, arc_page *pp)
{
	arc_page *p, *e;
	arc_list where;

	e = NULL;
	if ((arc->len[_T1_] >= 1) &&
		((pp && pp->where == _B2_ && arc->len[_T1_] == arc->T) || (arc->len[_T1_] > arc->T))) {
		p = arc_remove_lru(arc, _T1_);
		arc_list_add(arc, p, _B1_);
	} else {
		p = arc_remove_lru(arc, _T2_);
		arc_list_add(arc, p, _B2_);
	}

	if (p->dirty) 
	{
		/* p is being evicted, but it still in the bottom part of the cache
		   so, we copy it to another page, so the caller can still free it
        	*/
		e = arc_new_page(p->n, p->data);
		e->dirty = 1;
	}
	p->dirty = 0;
	return e;
}

/*
 * Returns 1 if hit
 */
arc_page *arc_search(arc_t *arc, uint64_t n)
{
	arc_page *p;

	arc->io++;
	p = ht_search(arc->ht, n);
	if (p && (p->where == _T1_ || p->where == _T2_)) { /* ARC(c) hit */
		arc_list_remove(arc, p);
		arc_list_add(arc, p, _T2_);
		arc->hits++;
		return p;
	}
	return NULL;
}

void arc_add(arc_t *arc, arc_page *new, arc_page **evicted)
{
	u_int64_t total_len;
	arc_page *p, *e, *pp;

	*evicted = e = NULL;
	p = ht_search(arc->ht, new->n);
	if (p && p->where == _B1_) { /* ARC(c) miss, DBL(2c) hit */
		arc->T = MIN(arc->C, (int64_t) arc->T + (int64_t) MAX(arc->len[_B2_] / arc->len[_B1_], 1));
		e = arc_replace(arc, p);
		arc_list_remove(arc, p);
		arc_list_add(arc, p, _T2_);
		p->data = new->data;
		FREE(new);
	} else if (p && p->where == _B2_) { /* ARC(c) miss, DBL(2c) hit */
		arc->T = MAX(0, (int64_t) arc->T - (int64_t) MAX(arc->len[_B1_] / arc->len[_B2_], 1));
		e = arc_replace(arc, p);
		arc_list_remove(arc, p);
		arc_list_add(arc, p, _T2_);
		p->data = new->data;
		FREE(new);
	} else if (!p) { /* ARC(c) miss, DBL(2c) miss */
		total_len = arc->len[_T1_] + arc->len[_T2_] + arc->len[_B1_] + arc->len[_B2_];
		if ((arc->len[_T1_] + arc->len[_B1_]) == arc->C) {
			if (arc->len[_T1_] < arc->C) {
				pp = arc_remove_lru(arc, _B1_); // already evicted: this is B1
				ht_remove(arc->ht, pp); FREE(pp);
				e = arc_replace(arc, p);
			} else {
				e = arc_remove_lru(arc, _T1_);
				ht_remove(arc->ht, e);
			}
		} else if ((arc->len[_T1_] + arc->len[_B1_]) < arc->C && total_len >= arc->C) {
			if (total_len == 2 * arc->C) {
				pp = arc_remove_lru(arc, _B2_);
				ht_remove(arc->ht, pp); FREE(pp);
			}
			e = arc_replace(arc, p);
		}
		arc_list_add(arc, new, _T1_);
		ht_add(arc->ht, new->n, new);
	}
	*evicted = e;
	if (e && e->dirty)
		arc->evictions++;
}

/* size change */
void arc_inc_size(arc_t *arc, uint64_t inc)
{
	_ASSERT(0); // not implemented
}

void arc_dec_size(arc_t *arc, arc_page **evicted)
{
	_ASSERT(0); // not implemented
}
