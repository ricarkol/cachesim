#include "list.h"
#include "hash.h"
#include "htable.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#define MALLOC(x)	malloc(x)
#define FREE(x)		free(x)
#define _ASSERT(x)	assert(x)

typedef struct item_t {
	void 		*item;
	dl_list		ht_links;
} item_t;

item_t *ht_new_item(void *a)
{
	item_t *t;
	t = MALLOC(sizeof(item_t));
	t->item = a;
	return t;
}

void ht_remove_item(item_t *t)
{
	dl_list_del(&t->ht_links);
	FREE(t);
}

htable_t *ht_init(uint64_t bits, ht_get_key_f get)
{
	uint64_t i;
	htable_t *ht;

	ht = MALLOC(sizeof(struct htable_t));
	if (!ht)
		return NULL;	
	ht->bits = bits;
	ht->size = 1 << bits;
	ht->get_key = get;
	ht->d = MALLOC(ht->size * sizeof(dl_list));
	if (!ht->d) {
		FREE(ht);
		return NULL;
	}
	for (i = 0; i < ht->size; i++) {
		dl_list_init(&ht->d[i]);
	}
	return ht;
}

void ht_free(htable_t *ht)
{
	dl_list *head;
	uint64_t i;
	item_t *t, *n;
	for (i = 0; i < ht->size; i++) {
		head = &(ht->d[i]);
		dl_list_for_each_safe(t, n, head, item_t, ht_links) {
			FREE(t->item);
			FREE(t);
		}
	}
	FREE(ht->d);
	FREE(ht);
}

item_t *ht_search_item(htable_t *ht, ht_key_t k)
{
	dl_list *head;
	unsigned int hash;
	item_t *t, *n;
	hash = hash_mem((char *) &k, sizeof(ht_key_t), ht->bits);
	_ASSERT(hash <= ht->size);
	head = &(ht->d[hash]);
	dl_list_for_each_safe(t, n, head, item_t, ht_links) {
		if (ht->get_key(t->item) == k)
			return t;
	}
	return NULL;
}

void *ht_search(htable_t *ht, ht_key_t k)
{
	item_t *t;
	t = ht_search_item(ht, k);
	return t ? t->item : NULL;
}

/* XXX costly, should add some kind of back pointer to the item_t
   owning this element a */
void ht_remove(htable_t *ht, void *a)
{
	item_t *t;
	t = ht_search_item(ht, ht->get_key(a));
	ht_remove_item(t);
}

void ht_add(htable_t *ht, ht_key_t k, void *a)
{
	unsigned int hash;
	item_t *t;
	t = ht_new_item(a);
	hash = hash_mem((char *) &k, sizeof(ht_key_t), ht->bits);
	_ASSERT(hash <= ht->size);
	dl_list_add(&ht->d[hash], &t->ht_links);
}
