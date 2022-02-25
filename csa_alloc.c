#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>

#include "csa_alloc.h"
#include "csa_error.h"

#ifndef DEBUG
#define DEBUG 1 // when TRUE a leak report is printed at exit
#endif

#ifndef DETERMINISTIC
#define DETERMINISTIC 0 // when TRUE all mallocs are converted to callocs (no garbage)
#endif

#ifndef RECORD_ALLOC_COUNT_AND_SIZE
#define RECORD_ALLOC_COUNT_AND_SIZE 1
#endif

#ifndef DUMP_ON_DUPLICATE
#define DUMP_ON_DUPLICATE 1
#endif

#if DEBUG
typedef struct alloc_info_struct {
	void *ptr;
	const char *msg;
	size_t size;
	char freed;
} AllocInfo;

static volatile AllocInfo *allocs = NULL;
static volatile int n_allocs = 0;

#if RECORD_ALLOC_COUNT_AND_SIZE
static volatile int alloc_count = 0;
static volatile long long int alloc_total_size = 0;
static volatile int free_count = 0;
static volatile long long int free_total_size = 0;
#endif

static sem_t threadlock;
#endif

void csa_init_alloc_tracker(void) {
#if DEBUG
	sem_init(&threadlock, 0, 1);
#endif
}

void csa_alloc_print_report(void)
{
#if DEBUG
	for(int i = 0; i < n_allocs; ++i){
		if(!allocs[i].freed){
			fprintf(stderr, "\033[1;33mLEAKED %p: %9lu bytes @ %s\033[0m\n", allocs[i].ptr, allocs[i].size, allocs[i].msg);
		}
	}
#if RECORD_ALLOC_COUNT_AND_SIZE
	long long int ats = alloc_total_size;
	long long int fts = free_total_size;
	int scale = 0;
	while(ats >= 1000000000LL || fts >= 1000000000LL){
		ats /= 1000LL;
		fts /= 1000LL;
		++scale;
	}
	fprintf(stderr,
		"      \tAllocated\t    Freed\n"
		"Allocs\t\033[1;34m%9i\t%9i\033[0m\n"
		"%sBytes%s\t\033[1;34m%9lli\t%9lli\033[0m\n"
		"%s\033[0m\n",
		alloc_count, free_count,
		scale == 0 ? "" : (scale == 1 ? "K" : (scale == 2 ? "M" : (scale == 3 ? "G" : "!"))), scale == 0 ? " " : "", ats, fts,
		alloc_count == free_count ? "\033[1;32mNo memory leaks!" : "\033[1;31mMemory leaks!"
	);
#endif
#endif
}

#if DEBUG
static void log_alloc(void *ptr, const char *msg, size_t size)
{
	sem_wait(&threadlock);
	
#if RECORD_ALLOC_COUNT_AND_SIZE
	++alloc_count;
	alloc_total_size += size;
#endif
	
	char found = 0;
	for(int i = 0; i < n_allocs; ++i){
		if(allocs[i].ptr == ptr){
			if(allocs[i].freed){
				allocs[i] = (AllocInfo){ptr, msg, size, 0};
				found = 1;
			}else{
				csa_error("POINTER %p ALLOCATED REPEATEDLY WITHOUT FREE! (%9lu bytes @ %s)\n", ptr, size, msg);
			}
		}
	}
	if(found){
		sem_post(&threadlock);
		return;
	}
	
	++n_allocs;
	allocs = realloc((void*)allocs, n_allocs * sizeof(AllocInfo));
	if(allocs == NULL){
		csa_error("ABORTING: failed to (re)allocate memory to record the allocation of memory for debugging purposes.\n");
		exit(-1);
	}
	allocs[n_allocs - 1] = (AllocInfo){ptr, msg, size, 0};
	
	sem_post(&threadlock);
}
#endif

void *_csa_calloc(const char *alloc_info, size_t nitems, size_t size)
{
#if DEBUG
	void *data = calloc(nitems, size);
	log_alloc(data, alloc_info, nitems * size);
	return data;
#else // !DEBUG
	(void)alloc_info; // unused
	return calloc(nitems, size);
#endif // END DEBUG
}

void *_csa_malloc(const char *alloc_info, size_t size)
{
#if DETERMINISTIC
	return _csa_calloc(alloc_info, 1, size); // when DETERMINISTIC=1, all mallocs become callocs (no junk, so program behaviour is deterministic)
#else // !DETERMINISTIC
#if DEBUG
	void *data = malloc(size);
	log_alloc(data, alloc_info, size);
	return data;
#else // !DEBUG
	(void)alloc_info; // unused
	return malloc(size);
#endif // END DEBUG
#endif // END DETERMINISTIC
}

void csa_free(void *ptr)
{
#if DEBUG
	if(ptr == NULL) return;
	
	sem_wait(&threadlock);
	
#if RECORD_ALLOC_COUNT_AND_SIZE
	++free_count;
#endif
	
	char found = 0;
	for(int i = 0; i < n_allocs; ++i){
		if(allocs[i].ptr == ptr){
			if(found){
				csa_error("POINTER %p OCCURS MORE THAN ONCE IN ARRAY 'allocs'! (this occurrence: %lu bytes @ %s)\n", ptr, allocs[i].size, allocs[i].msg);
#if DUMP_ON_DUPLICATE
				for(int j = 0; j < n_allocs; ++j){
					fprintf(stderr, "%s%p\t %i %9lu %s\033[0m\n", allocs[j].ptr == ptr ? "\033[1;31m" : "", allocs[j].ptr, allocs[j].freed, allocs[j].size, allocs[j].msg);
				}
				fprintf(stderr, "\n");
#endif
			}
			if(allocs[i].freed){
				csa_error("POINTER %p (%lu bytes @ %s) IS BEING FREED A SECOND TIME!\n", ptr, allocs[i].size, allocs[i].msg);
			}else{
				allocs[i].freed = 1;
#if RECORD_ALLOC_COUNT_AND_SIZE
				if(!found){
					free_total_size += allocs[i].size;
				}
#endif
			}
			found = 1;
		}
	}
	if(!found){
		csa_error("POINTER %p WAS NOT FOUND IN 'allocs' ARRAY!\n", ptr);
	}
	
	sem_post(&threadlock);
#endif
	free(ptr);
}
