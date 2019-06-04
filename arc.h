#ifndef ARC_H
#define ARC_H

#include "list.h"
#include "htable.h"

#include <sys/types.h>
#include <stdint.h>

typedef enum {_T1_ = 0, _T2_ = 1, _B1_ = 2, _B2_ = 3} arc_list;

typedef struct arc_page {
	uint64_t		n;
	uint64_t		data;
	arc_list		where;   /* what arc list */
	int32_t			dirty;
	dl_list           	links;
} arc_page;

typedef struct arc_t {
	/* pages are searched with a hash table */
	htable_t		*ht;
	dl_list           	links[4];
	uint64_t		len[4];
	uint64_t		C;   /* cache capacity */
	uint64_t		T;   /* ARC target, p in the paper */
	/* Statistics */
	uint64_t		io;
	uint64_t		hits;
	uint64_t		evictions; /* dirty evictions */
} arc_t;

arc_t *arc_init(uint64_t);
arc_page *arc_new_page(uint64_t n, uint64_t data);
void arc_add(arc_t *arc, arc_page *new, arc_page **evicted);
arc_page *arc_search(arc_t *arc, uint64_t n);
void arc_print(arc_t *);

/* size change */
void arc_inc_size(arc_t *arc, uint64_t inc);
void arc_dec_size(arc_t *arc, arc_page **evicted);

#endif
