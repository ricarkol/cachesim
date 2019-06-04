#ifndef TABLE_H
#define TABLE_H

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include "list.h"

typedef int64_t ht_key_t;

typedef ht_key_t (*ht_get_key_f)(void *a);
typedef void (*ht_set_back_f)(void *a);


typedef struct htable_t {
	int64_t		bits;
	int64_t		size;
	ht_get_key_f	get_key;
	dl_list		*d;
} htable_t;

htable_t *ht_init(uint64_t bits, ht_get_key_f get);
void ht_free(htable_t *ht);
void *ht_search(htable_t *ht, ht_key_t key);
void ht_add(htable_t *ht, ht_key_t key, void *a);
void ht_remove(htable_t *ht, void *a);

#endif
