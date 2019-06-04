#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include "hash.h"
#include "htable.h"
#include "list.h"


typedef struct min_t {
	/* pages are searched with a hash table */
	htable_t		*htp;
	/* a hash table to maintain counters for index values */
	htable_t		*hti;
	u_int64_t		c;	/* cache capacity */
	u_int64_t		curr;	/* MIN current register */
	u_int64_t		comp;	/* MIN complete counter */
	/* Statistics */
	u_int64_t		io;
	u_int64_t		hits;
	u_int64_t		evictions; /* dirty evictions */
} min_t;

typedef struct min_page {
	u_int64_t		n;
	u_int64_t		index;	/* MIN block index */
	int			dirty;
} min_page;

min_page *min_new_page(u_int64_t n)
{
	min_page *p;
	p = malloc(sizeof(min_page));
	p->n = n;
	p->index = 0;
	p->dirty = 9;
	return p;
}

ht_key_t htp_get_key(void *a)
{
	return (ht_key_t) ((min_page *) a)->n;
}

typedef struct index_count {
	u_int64_t		index;
	u_int64_t		count;	/* count of pages with this index */
} index_count;

ht_key_t hti_get_key(void *a)
{
	return (ht_key_t) ((index_count *) a)->index;
}

index_count *new_index_count(u_int64_t i)
{
	index_count *ic;
	ic = malloc(sizeof(index_count));
	ic->index = i;
	ic->count = 0;
	return ic;
}


void hti_count_inc(min_t *min, u_int64_t index)
{
	index_count *ic;
	ic = ht_search(min->hti, index);
	if (ic)
		ic->count++;
	else {
		ic = new_index_count(index);
		ic->count++;
		ht_add(min->hti, index, ic);
	}
}

void hti_count_dec(min_t *min, u_int64_t index)
{
	index_count *ic;
	ic = ht_search(min->hti, index);
	if (!ic) {
		/* if there is no counter for
		   this index, just return */
		return;
	}
	ic->count--;
	if (ic->count == 0) {
		ht_remove(min->hti, ic);
		free(ic);
	}
}

u_int64_t hti_count(min_t *min, u_int64_t index)
{
	index_count *ic;
	ic = ht_search(min->hti, index);
	if (ic)
		return ic->count;
	return 0;
}


min_t *min_init(u_int64_t cc)
{
	min_t *min;
	min = malloc(sizeof(min_t));
	if (!min)
		return NULL;
	min->c = cc;
	min->curr = 0;
	min->comp = 1;
	min->htp = ht_init(20, htp_get_key);
	if (!min->htp) {
		free(min);
		return NULL;
	}
	min->hti = ht_init(20, hti_get_key);
	if (!min->hti) {
		free(min->htp);
		free(min);
		return NULL;
	}
	min->io = 0;
	min->hits = 0;
	min->evictions = 0;
	return min;
}

void min_free(min_t *min)
{
	ht_free(min->htp);
	ht_free(min->hti);
	free(min);
}

/*
 * Returns 1 if hit
 */
int min_access(min_t *min, char type, u_int64_t n)
{
	u_int64_t counter = 0;	/* MIN counter */
	u_int64_t temp = 0;		/* MIN temporary register */
	min_page *p;

	min->io++;

	p = ht_search(min->htp, n);
	if (!p) {
		p = min_new_page(n);
		ht_add(min->htp, n, p);
	}

	/* Step 1 */
	if (p->index < min->comp) {
		min->curr++; /* Advance */
		/* decrement the counter for previous index
		   and increment it for current index */
		hti_count_dec(min, p->index);
		p->index = min->curr;
		hti_count_inc(min, p->index);
		return 0;
	}

	if (p->index == min->curr) {
		min->hits++;
		return 1;
	}

	if (p->index < min->curr && p->index >= min->comp) {
		/* decrement the counter for previous index
		   and increment it for current index */
		hti_count_dec(min, p->index);
		p->index = min->curr;
		hti_count_inc(min, p->index);
		temp = min->curr;
		counter = 0;
		
step_2:		/* Step 2 */
		counter += hti_count(min, temp);
		if (counter == min->c || temp == min->comp) {
			min->comp = temp;
			min->hits++;
			return 1;
		}

		if (counter < min->c) {
			counter--;
			temp--;
			goto step_2;
		}

		assert(1); /* NOT REACHABLE */
	}
	
	min->hits++;
	return 1;
}


int main(int argc, char *argv[])
{
	u_int64_t lbn = 0;
	u_int64_t s = 0;
	char type;
	min_t *min;

	if (argc < 1) {
		fprintf(stderr, "Usage: %s <SIZE>\n", argv[0]);
		return -1;
	}

	s = atoi(argv[1]);
	assert(s > 0);

	if ((min = min_init(s)) == NULL) {
		perror("Could not allocate memory");
		return 1;		
	}

	while (!feof(stdin)) {
		scanf("%c %llu\n", &type, &lbn);
		min_access(min, type, lbn);
	}

	printf("%llu %f %llu %llu %llu %llu\n", 
	s, (float) min->hits / (float) min->io,
	min->io - min->hits, min->hits, min->io,
	(min->io - min->hits) - s);
	min_free(min);
	return 0;
}
