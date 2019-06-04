#ifndef MRU_H
#define MRU_H

#include "list.h"
#include "htable.h"

#include <sys/types.h>
#include <stdint.h>


typedef struct mru_page {
	uint64_t		n;
	dl_list			links;
	int32_t			dirty;
	uint64_t		data;
} mru_page;


typedef struct mru_t {
	dl_list			links;
	uint64_t		len;	/* cur cache len */
	uint64_t		c;   	/* cache capacity */
	htable_t		*ht;	/* pages are searched with a hash table */
	/* Statistics */
	uint64_t		io;
	uint64_t		hits;
	uint64_t		evictions; /* dirty evictions */
} mru_t;


mru_t *mru_init(uint64_t cc);
void mru_free(mru_t *mru);
mru_page *mru_new_page(uint64_t n, uint64_t data);
void mru_add(mru_t *mru, mru_page *new, mru_page **evicted);
mru_page *mru_search(mru_t *mru, uint64_t n);

/* size change */
void mru_inc_size(mru_t *mru, uint64_t inc);
void mru_dec_size(mru_t *mru, mru_page **evicted);

#endif
