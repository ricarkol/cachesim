#ifndef LRU_H
#define LRU_H

#include "list.h"
#include "htable.h"

#include <sys/types.h>
#include <stdint.h>


typedef struct lru_page {
	uint64_t		n;
	uint64_t		data;
	dl_list			links;
	int32_t			dirty;
} lru_page;


typedef struct lru_t {
	dl_list			links;
	uint64_t		len;	/* cur cache len */
	uint64_t		c;   	/* cache capacity */
	struct htable_t		*ht;	/* pages are searched with a hash table */
	/* Statistics */
	uint64_t		io;
	uint64_t		hits;
	uint64_t		evictions; /* dirty evictions */
} lru_t;


lru_t *lru_init(uint64_t cc);
void lru_free(lru_t *lru);
lru_page *lru_new_page(uint64_t n, uint64_t data);
void lru_add(lru_t *lru, lru_page *new, lru_page **evicted);
lru_page *lru_search(lru_t *lru, uint64_t n);

/* size change */
void lru_inc_size(lru_t *lru, uint64_t inc);
void lru_dec_size(lru_t *lru, lru_page **evicted);

#endif
