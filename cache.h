#ifndef CACHE_H
#define CACHE_H

#if (ALG == 1)
	#include "lru.h"
	#define ALG_T		lru_t
	#define ALG_PAGE	lru_page
	#define ALG_NEW_PAGE	lru_new_page
	#define ALG_INIT	lru_init
	#define ALG_SEARCH	lru_search
	#define ALG_ADD		lru_add
	#define ALG_FREE	lru_free
#elif (ALG == 2)
	#include "mru.h"
	#define ALG_T		mru_t
	#define ALG_PAGE	mru_page
	#define ALG_NEW_PAGE	mru_new_page
	#define ALG_INIT	mru_init
	#define ALG_SEARCH	mru_search
	#define ALG_ADD		mru_add
	#define ALG_FREE	mru_free
#elif (ALG == 3)
	#include "arc.h"
	#define ALG_T		arc_t
	#define ALG_PAGE	arc_page
	#define ALG_NEW_PAGE	arc_new_page
	#define ALG_INIT	arc_init
	#define ALG_SEARCH	arc_search
	#define ALG_ADD		arc_add
	#define ALG_FREE	arc_free
#endif

#endif
