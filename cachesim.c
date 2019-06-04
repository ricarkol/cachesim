#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include "cache.h"

int main(int argc, char *argv[])
{
	u_int64_t lbn = 0;
	u_int64_t s = 0;
	char type;
	ALG_T *cache;
	ALG_PAGE *p, *e;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <SIZE>\n", argv[0]);
		return -1;
	}

	s = atoi(argv[1]);
	//assert(s > 0);

	if ((cache = ALG_INIT(s)) == NULL) {
		perror("Could not allocate memory");
		return 1;		
	}

	while (!feof(stdin)) {
		scanf("%c %llu\n", &type, &lbn);
		p = ALG_SEARCH(cache, lbn);
		if (!p) {	// miss
			p = ALG_NEW_PAGE(lbn, 0xdeadf00dul);
			ALG_ADD(cache, p, &e);
			if (e)
				free(e);
		}
		if (type == 'W')
			p->dirty = 1;
	}

	printf("%llu %f %llu %llu %llu %llu\n", 
	s, (float) cache->hits / (float) cache->io,
	cache->io - cache->hits, cache->hits, cache->io, cache->evictions);
	ALG_FREE(cache);
	return 0;
}
