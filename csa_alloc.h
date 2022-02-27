#ifndef CPT_SONAR_ASSIST_ALLOC_H
#define CPT_SONAR_ASSIST_ALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

int csa_init_alloc_tracker(void);
void csa_alloc_print_report(void);
void *_csa_calloc(const char *alloc_info, size_t nitems, size_t size);
void *_csa_malloc(const char *alloc_info, size_t size);
void csa_free(void *ptr);

#ifdef __cplusplus
}
#endif

#define STR_(s) #s
#define STR(s) STR_(s)

#define csa_calloc(nitems, size) (_csa_calloc(__FILE__ ":" STR(__LINE__), (size_t)(nitems), (size_t)(size)))
#define csa_malloc(size)         (_csa_malloc(__FILE__ ":" STR(__LINE__), (size_t)(size)))

#endif
