#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <sys/types.h>
#include "pqueue.h"
#include "htable.h"
#include "list.h"


typedef struct opt_t {
	u_int64_t		len; /* cur cache len */
	u_int64_t		c;   /* cache capacity */
	pqueue_t		*pq;
	/* pages are searched with a hash table */
	htable_t		*ht;
	/* Statistics */
	u_int64_t		io;
	u_int64_t		hits;
	u_int64_t		evictions; /* dirty evictions */
} opt_t;

typedef struct opt_page {
	u_int64_t		n;
	u_int64_t		pq_next; 	/* pqueue priority */
	u_int64_t		pq_pos;		/* pqueue pos */
	int			dirty;
} opt_page;

ht_key_t opt_get_key(void *a)
{
	return (ht_key_t) ((opt_page *) a)->n;
}

static int cmp_pri(u_int64_t next, u_int64_t curr)
{
	return (next <= curr);
}


static u_int64_t get_pri(void *a)
{
	return (u_int64_t) ((opt_page *) a)->pq_next;
}


static void set_pri(void *a, u_int64_t pri)
{
	((opt_page *) a)->pq_next = pri;
}


static u_int64_t get_pos(void *a)
{
	return ((opt_page *) a)->pq_pos;
}


static void set_pos(void *a, u_int64_t pos)
{
	((opt_page *) a)->pq_pos = pos;
}


opt_page *opt_new_page(u_int64_t n, u_int64_t next)
{
	opt_page *p;
	p = malloc(sizeof(opt_page));
	if (!p)
		return NULL;
	p->n = n;
	p->pq_next = next;
	p->pq_pos = 0;
	return p;
}

opt_t *opt_init(u_int64_t cc)
{
	opt_t *opt;
	opt = malloc(sizeof(opt_t));
	if (!opt)
		return NULL;
	opt->c = cc;
	opt->len = 0;
	opt->pq = pqueue_init(opt->c, cmp_pri, get_pri, set_pri, get_pos, set_pos);
	opt->ht = ht_init(20, opt_get_key);
	opt->io = 0;
	opt->hits = 0;
	opt->evictions = 0;
	if (!opt->ht || !opt->pq)
		return NULL;
	return opt;
}

void opt_free(opt_t *opt)
{
	pqueue_free(opt->pq);
	ht_free(opt->ht);
	free(opt);
}

/*
 * Returns 1 if hit
 */
int opt_access(opt_t *opt, char type, u_int64_t n, u_int64_t i, u_int64_t next)
{
	unsigned int hash;
	opt_page *p;
	int hit = 0;

	opt->io++;
	p = ht_search(opt->ht, n);
	if (p) {	/* Hit */
		pqueue_change_priority(opt->pq, next, p);
		opt->hits++;
		hit = 1;
	} else {	/* Miss */
		if (opt->len == opt->c) {
			/* evict */
			p = pqueue_pop(opt->pq);
			ht_remove(opt->ht, p);
			if (p->dirty)
				opt->evictions++;
			free(p);
			opt->len--;
		}
		p = opt_new_page(n, next);
		pqueue_insert(opt->pq, p);
		ht_add(opt->ht, n, p);
		opt->len++;
	}
	if (type == 'W')
		p->dirty = 1;
	return hit;
}

int main(int argc, char *argv[])
{
	u_int64_t lbn = 0;
	u_int64_t i = 0;
	u_int64_t next = 0;
	u_int64_t s = 0;
	char type;
	opt_t *opt;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <SIZE>\n", argv[0]);
		return -1;
	}

	s = atoi(argv[1]);
	assert(s > 0);

	if ((opt = opt_init(s)) == NULL) {
		perror("Could not allocate memory");
		return 1;		
	}

	while (!feof(stdin)) {
		scanf("%c %llu %llu %llu\n", &type, &lbn, &i, &next);
		opt_access(opt, type, lbn, i, next == 0 ? ULONG_MAX : next);
	}

	printf("%llu %f %llu %llu %llu %llu\n", 
	s, (float) opt->hits / (float) opt->io,
	opt->io - opt->hits, opt->hits, opt->io, opt->evictions);
	opt_free(opt);
	return 0;
}
