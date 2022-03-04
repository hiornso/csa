#include <gtk/gtk.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <math.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define HAVE_INLINE 1 // use inlines for GSL where possible

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_blas.h>

extern "C" {

#include "main.h"
#include "tracker.h"
#include "accelerated.h"
#include "csa_error.h"

}

#define SQUARE(x) ((x)*(x))

#define USE_MULTITHREADING 1
#ifndef USE_VECTORS
#define USE_VECTORS 0
#endif

#define THREADS 8
#define VECSIZE 8

#define DISABLE_BICUBIC 0
#define DISABLE_COMPOSITE 0

#if USE_MULTITHREADING
#include <pthread.h>

pthread_t threads[THREADS];
pthread_mutex_t render_lock;
pthread_mutex_t subthread_lock_run[THREADS];
pthread_mutex_t subthread_lock_done[THREADS];
#endif

#if USE_VECTORS
#include "vcl/vectorclass.h"

#if VECSIZE == 16
typedef Vec16fb Vecfb;
typedef Vec16f Vecf;
typedef Vec16i Veci;
#define ASCENDING_VECTOR Veci(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15)
#elif VECSIZE == 8
typedef Vec8fb Vecfb;
typedef Vec8f Vecf;
typedef Vec8i Veci;
#define ASCENDING_VECTOR Veci(0,1,2,3,4,5,6,7)
#elif VECSIZE == 4
typedef Vec4fb Vecfb;
typedef Vec4f Vecf;
typedef Vec4i Veci;
#define ASCENDING_VECTOR Veci(0,1,2,3)
#else
#error Invalid VECSIZE, must be one of {4,8,16}
#endif

#endif // USE_VECTORS

typedef struct bicubic_matrix {
    float mat[4*4];
} BicubicMatrix;

#if USE_MULTITHREADING

typedef struct thread_arg_bicubic {
    float **storage;
    BicubicMatrix *matrices;
    float *big;
    int s;
    int res;
    int offset;
} ThreadArgBicubic;

typedef struct thread_arg_composite {
    Tracker *tracker;
    MapLayerColourMapping *mappings;
    int res;
    int stride;
    int layerCount;
    int layerSize;
} ThreadArgComposite;

enum thread_task {
    EXIT_THREAD,
    BICUBIC,
    COMPOSITE,
};

typedef struct thread_arg_generic {
    enum thread_task task;
    union {
        ThreadArgBicubic bicubic;
        ThreadArgComposite composite;
    } thread_arg;
} ThreadArg;

static atomic_char32_t render_square;
static ThreadArg thread_arg;

static void launch_render(ThreadArg ta, int nthreads)
{
    pthread_mutex_lock(&render_lock); // wait for any other rendering processes to finish
    
    thread_arg = ta;
    atomic_thread_fence(memory_order_release);
    
    atomic_init(&render_square, 0);
    
    for(int i = 0; i < nthreads; ++i) pthread_mutex_unlock(&subthread_lock_run[i]); // send a start rendering message to each thread
    for(int i = 0; i < nthreads; ++i) pthread_mutex_lock(&subthread_lock_done[i]);
    for(int i = 0; i < nthreads; ++i) pthread_mutex_lock(&subthread_lock_run[i]); // wait for all threads to finish rendering
    for(int i = 0; i < nthreads; ++i) pthread_mutex_unlock(&subthread_lock_done[i]);
    
    pthread_mutex_unlock(&render_lock); // mark that rendering is finished
}

#endif

#if OPENBLAS
extern "C" void openblas_set_num_threads(int threads);
#endif

static const float constmat[] = {
     1, 0, 0, 0,
     0, 0, 1, 0,
    -3, 3,-2,-1,
     2,-2, 1, 1,
};

static void bicubic_square(float **storage, const BicubicMatrix *matrices, const int s, float *big, const int res, const int offset, const int square)
{
    const int size = s + 2 * offset - 1;
    const int row = square / size;
    const int col = square % size;
    
    const BicubicMatrix *mat = &matrices[square];
    
    int square_start_x;
    int square_end_x;
    int square_start_y;
    int square_end_y;
    
    if (offset) {
        square_start_x = (int)((col - 1) * (float)res / s + 0.5f * (float)res / s);
        if (square_start_x < 0) square_start_x = 0;
        square_end_x = (int)(col * (float)res / s + 0.5f * (float)res / s);
        if (square_end_x >= res) square_end_x = res - 1;
        square_start_y = (int)((row - 1) * (float)res / s + 0.5f * (float)res / s);
        if (square_start_y < 0) square_start_y = 0;
        square_end_y = (int)(row * (float)res / s + 0.5f * (float)res / s);
        if (square_end_y >= res) square_end_y = res - 1;
    } else {
        square_start_x = (int)(col * (float)res / (s - 1));
        square_end_x = (int)((col + 1) * (float)res / (s - 1));
        square_start_y = (int)(row * (float)res / (s - 1));
        square_end_y = (int)((row + 1) * (float)res / (s - 1));
    }
    
    const int res_x = square_end_x - square_start_x;
    const int res_y = square_end_y - square_start_y;
    
    float *ymat = storage[0];
    float *tmp  = storage[1];
    float *xmat = storage[2];
    
    for(int j = 0; j < 4; ++j) {
        for(int i = 0; i < res_y; ++i) {
            float f = 1.0f;
            float yy;
            int y = square_start_y + i;
            if(offset){
                yy = (( y - row * res / s + res/(2 * s) ) * s ) / (float)res;
            }else{
                yy = ((s - 1) * y - row * res) / (float)res;
            }
            for(int k = 0; k < j; ++k) {
                f *= yy;
            }
            ymat[j * res_y + i] = f;
        }
    }
    
    for(int i = 0; i < res_x; ++i) {
        for(int j = 0; j < 4; ++j) {
            float f = 1.0f;
            float xx;
            int x = square_start_x + i;
            if(offset){
                xx = (( x - col * res / s + res/(2 * s) ) * s ) / (float)res;
            }else{
                xx = ((s - 1) * x - col * res) / (float)res;
            }
            for(int k = 0; k < j; ++k) {
                f *= xx;
            }
            xmat[i * 4 + j] = f;
        }
    }
    
    gsl_matrix_float_const_view x_view = gsl_matrix_float_const_view_array(xmat, res_x, 4);
    gsl_matrix_float_const_view y_view = gsl_matrix_float_const_view_array(ymat, 4, res_y);
    gsl_matrix_float_view tmp_view = gsl_matrix_float_view_array(tmp, 4, res_y);
    gsl_matrix_float_const_view bicubic = gsl_matrix_float_const_view_array(mat->mat, 4, 4);
    gsl_matrix_float_view dest = gsl_matrix_float_view_array_with_tda(&big[square_start_y * res + square_start_x], res_y, res_x, res);
    
    gsl_blas_sgemm(CblasNoTrans, CblasNoTrans, 1.0f, &bicubic.matrix, &y_view.matrix, 0.0f, &tmp_view.matrix);
    gsl_blas_sgemm(CblasTrans, CblasTrans, 1.0f, &tmp_view.matrix, &x_view.matrix, 1.0f, &dest.matrix);
}

#if USE_MULTITHREADING

static void maintain_thread_bicubic_square(int thread_index)
{
    ThreadArgBicubic ta = thread_arg.thread_arg.bicubic;
    int square;
    while((square = atomic_fetch_add_explicit(&render_square, 1, memory_order_relaxed)) < SQUARE(ta.s + 2 * ta.offset - 1)){
        bicubic_square(ta.storage + 3 * thread_index, ta.matrices, ta.s, ta.big, ta.res, ta.offset, square);
    }
}

#endif

int32_t bicubic(float *small, int32_t s, float *big, int32_t res, int32_t offset)
{
#if !DISABLE_BICUBIC

    offset = offset ? 1 : 0;
    const int size = s + 2 * offset - 1;
    BicubicMatrix *matrices = (BicubicMatrix*)csa_calloc(size * size, sizeof(BicubicMatrix)); // could cache this, but probably not worth it - not a hot loop, and would increase memory usage a bit, so why bother
    if(matrices == NULL){
        return -1;
    }
    
    gsl_matrix_float_const_view constmat_view = gsl_matrix_float_const_view_array(constmat, 4, 4);
    for(int row = 1 - offset; row < s + offset; ++row){
        for(int col = 1 - offset; col < s + offset; ++col){
            
#define F(Y,X) (small[CLAMP((Y) - 1, 0, s - 1) * s + CLAMP((X) - 1, 0, s - 1)])
#define Fx(Y,X) ((F((Y), (X) + 1) - F((Y), (X) - 1))/2.0f)
#define Fy(Y,X) ((F((Y) + 1, (X)) - F((Y) - 1, (X)))/2.0f)
#define Fxy(Y,X) ((Fy((Y), (X) + 1) - Fy((Y), (X) - 1))/2.0f)
            
            float tmp_[4*4];
            const float values_[] = { // transposed here so we can transpose again later (sgemm faster when second mat. is transposed)
                F (row    , col    ), F (row    , col + 1), Fx (row    , col    ), Fx (row    , col + 1),
                F (row + 1, col    ), F (row + 1, col + 1), Fx (row + 1, col    ), Fx (row + 1, col + 1),
                Fy(row    , col    ), Fy(row    , col + 1), Fxy(row    , col    ), Fxy(row    , col + 1),
                Fy(row + 1, col    ), Fy(row + 1, col + 1), Fxy(row + 1, col    ), Fxy(row + 1, col + 1),
            };
            
            gsl_matrix_float_view tmp = gsl_matrix_float_view_array(tmp_, 4, 4);
            gsl_matrix_float_const_view values = gsl_matrix_float_const_view_array(values_, 4, 4);
            gsl_matrix_float_view dest = gsl_matrix_float_view_array(matrices[(row + offset - 1) * size + (col + offset - 1)].mat, 4, 4);
            
            gsl_blas_sgemm(
                CblasNoTrans, CblasTrans,
                1.0f, &constmat_view.matrix, &values.matrix,
                0.0f, &tmp.matrix
            ); // tmp = 1.0 * constmat * values + 0.0 * tmp
            
            gsl_blas_sgemm(
                CblasNoTrans, CblasTrans,
                1.0f, &tmp.matrix, &constmat_view.matrix,
                0.0f, &dest.matrix
            ); // dest = 1.0 * tmp * constmat^T + 0.0 * dest;
        }
    }
    
    int max_res = ceilf((float)res / (offset ? s : s - 1));
    
#if USE_MULTITHREADING
    float *storage[3*THREADS];
    for (int i = 0; i < 3*THREADS; ++i) {
        storage[i] = (float*)csa_malloc(sizeof(float) * 4 * max_res);
        if (storage[i] == NULL) {
            for (int j = 0; j < i; ++j) {
                csa_free(storage[j]);
            }
            return -1;
        }
    }
    
    ThreadArg ta;
    ta.task = BICUBIC;
    ta.thread_arg.bicubic = /*(ThreadArgBicubic)*/{
        storage,
        matrices,
        big,
        s,
        res,
        offset
    };
    
    launch_render(ta, THREADS);
    
    for(int i = 0; i < 3*THREADS; ++i) {
        csa_free(storage[i]);
    }
#else
    float *storage[3];
    for (int i = 0; i < 3; ++i) {
        storage[i] = (float*)csa_malloc(sizeof(float) * 4 * max_res);
        if (storage[i] == NULL) {
            for (int j = 0; j < i; ++j) {
                csa_free(storage[j]);
            }
            return -1;
        }
    }
    
    for(int square = 0; square < size * size; ++square){
        bicubic_square(storage, matrices, s, big, res, offset, square);
    }
    
    for(int i = 0; i < 3; ++i) {
        csa_free(storage[i]);
    }
#endif

    csa_free(matrices);

#endif
	
	return 0;
}

static void composite_row(Tracker *tracker, MapLayerColourMapping *mappings, int res, int stride, int layerCount, int layerSize, int y)
{
    int done_with_vectors = 0;
#if USE_VECTORS
    int iters = res / VECSIZE;
    done_with_vectors = iters * VECSIZE;
    
    Veci x = ASCENDING_VECTOR;
    for(int k = 0; k < iters; ++k){
        Vecf r(0.f),g(0.f),b(0.f);
        Vecfb done = Vecfb(false);
        for(int i = 0; i < layerCount; ++i){
            Vecf w = 255.f * lookup<INT_MAX>(x, (const float *)(tracker->mrc.vals + layerSize * i + y * res)); // height map
            
            MapLayerColourMapping m = mappings[i];
            
            if(m.mapping_type == CONTINUOUS){
                
                r = select(done, r, m.mappings.continuous.mr * w + m.mappings.continuous.cr);
                g = select(done, g, m.mappings.continuous.mg * w + m.mappings.continuous.cg);
                b = select(done, b, m.mappings.continuous.mb * w + m.mappings.continuous.cb);
                
                break;
            }else{ // STEPPED
                w = select(w > m.mappings.stepped.wraparound, 2.f * m.mappings.stepped.wraparound - w, w);
                for(int j = 0; j < m.mappings.stepped.len; ++j){
                    Vecfb cond = (~done) & (w > m.mappings.stepped.transitions[j].threshold);
                    done |= cond;
                    r = select(cond, m.mappings.stepped.transitions[j].c.r, r);
                    g = select(cond, m.mappings.stepped.transitions[j].c.g, g);
                    b = select(cond, m.mappings.stepped.transitions[j].c.b, b);
                }
            }
            
        }
        
        for(int j = 0; j < VECSIZE; ++j){
            tracker->mrc.pixels[y * stride + (k * VECSIZE + j) * 4 + 2] = (unsigned char)r[j]; // r
            tracker->mrc.pixels[y * stride + (k * VECSIZE + j) * 4 + 1] = (unsigned char)g[j]; // g
            tracker->mrc.pixels[y * stride + (k * VECSIZE + j) * 4 + 0] = (unsigned char)b[j]; // b
        }
        
        x += VECSIZE;
    }
#endif
    for(int x = done_with_vectors; x < res; ++x){
        Colour c = /*(Colour)*/{0.0f, 0.0f, 0.0f};
        for(int i = 0; i < layerCount; ++i){
            float w = 255.f * tracker->mrc.vals[layerSize * i + y * res + x]; // height map
            
            MapLayerColourMapping m = mappings[i];
            
            if(m.mapping_type == CONTINUOUS){
                c = /*(Colour)*/{
                    m.mappings.continuous.mr * w + m.mappings.continuous.cr,
                    m.mappings.continuous.mg * w + m.mappings.continuous.cg,
                    m.mappings.continuous.mb * w + m.mappings.continuous.cb
                };
                break;
            }else{ // STEPPED
                if(w > m.mappings.stepped.wraparound){
                    w = 2.f * m.mappings.stepped.wraparound - w;
                }
                for(int j = 0; j < m.mappings.stepped.len; ++j){
                    if(w > m.mappings.stepped.transitions[j].threshold){
                        c = m.mappings.stepped.transitions[j].c;
                        goto endloop;
                    }
                }
            }
        }
        endloop:
        
        tracker->mrc.pixels[y * stride + x * 4 + 2] = (unsigned char)c.r; // r
        tracker->mrc.pixels[y * stride + x * 4 + 1] = (unsigned char)c.g; // g
        tracker->mrc.pixels[y * stride + x * 4 + 0] = (unsigned char)c.b; // b
    }
}

#if USE_MULTITHREADING

static void maintain_thread_composite_row(__attribute__((unused)) int thread_index)
{
    ThreadArgComposite ta = thread_arg.thread_arg.composite;
    int row;
    while((row = atomic_fetch_add_explicit(&render_square, 1, memory_order_relaxed)) < ta.res){
        composite_row(ta.tracker, ta.mappings, ta.res, ta.stride, ta.layerCount, ta.layerSize, row);
    }
}

#endif

void composite(Tracker *tracker, MapLayerColourMapping *mappings, int res, int stride, int layerCount, int layerSize)
{
#if !DISABLE_COMPOSITE

#if USE_MULTITHREADING
    ThreadArg ta;
    ta.task = COMPOSITE;
    ta.thread_arg.composite = /*(ThreadArgComposite)*/{
        tracker,
        mappings,
        res,
        stride,
        layerCount,
        layerSize
    };
    
    launch_render(ta, THREADS);
#else
    for(int y = 0; y < res; ++y){
        composite_row(tracker, mappings, res, stride, layerCount, layerSize, y);
    }
#endif

#endif
}

#if USE_MULTITHREADING

__attribute__((noreturn)) static void* maintain_thread_generic(void *data)
{
    const int thread_index = (int)(intptr_t)data;
    pthread_mutex_lock(&subthread_lock_done[thread_index]);
    while(1){
        pthread_mutex_lock(&subthread_lock_run[thread_index]); // wait to be told to start
        pthread_mutex_unlock(&subthread_lock_done[thread_index]);
        atomic_thread_fence(memory_order_acquire); // get updates to thread_arg
        
        switch(thread_arg.task) {
            case EXIT_THREAD:
                pthread_mutex_unlock(&subthread_lock_run[thread_index]);
                pthread_exit(0);
            case BICUBIC:
                maintain_thread_bicubic_square(thread_index);
                break;
            case COMPOSITE:
                maintain_thread_composite_row(thread_index);
                break;
            default:
                csa_error("invalid value of 'thread_arg.task'. (Expected one of BICUBIC, COMPOSITE)\n");
        }
        
        pthread_mutex_unlock(&subthread_lock_run[thread_index]); // inform the main thread that rendering is done
        pthread_mutex_lock(&subthread_lock_done[thread_index]);
    }
}

static int destroy_threads(int nthreads)
{
    ThreadArg ta;
    ta.task = EXIT_THREAD;
    
    launch_render(ta, nthreads);
    
    for (int i = 0; i < nthreads; ++i) {
        pthread_mutex_unlock(&subthread_lock_run[i]);
    }
    
    for (int i = 0; i < nthreads; ++i) {
        pthread_join(threads[i], NULL);
    }
    
    return 0;
}

static int destroy_mutexes(bool free_render_lock, size_t free_subthread_lock_run, size_t free_subthread_lock_done)
{
    int status;
    int ret = 0;
    if (free_render_lock) {
        if ((status = pthread_mutex_destroy(&render_lock))) {
            csa_error("failed to deallocate 'render_lock' mutex: %s\n", strerror(status));
            --ret;
        }
    }
    for (size_t i = 0; i < free_subthread_lock_run; ++i) {
        if ((status = pthread_mutex_destroy(&subthread_lock_run[i]))) {
            csa_error("failed to deallocate 'subthread_lock_run[%i]' mutex: %s\n", i, strerror(status));
            --ret;
        }
    }
    for (size_t i = 0; i < free_subthread_lock_done; ++i) {
        if ((status = pthread_mutex_destroy(&subthread_lock_done[i]))) {
            csa_error("failed to deinitialise 'subthread_lock_done[%i]' mutex: %s\n", i, strerror(status));
            --ret;
        }
    }
    return ret;
}

#endif

int init_accelerate()
{
#if ACCELERATE_FRAMEWORK
    setenv("VECLIB_MAXIMUM_THREADS", "1", 0);
#endif
#if OPENBLAS
    openblas_set_num_threads(1);
#endif

#if USE_MULTITHREADING
    int status;
    if ((status = pthread_mutex_init(&render_lock, NULL))) {
        csa_error("failed to initialise 'render_lock' mutex: %s\n", strerror(status));
        destroy_mutexes(true, 0, 0);
        return -1;
    }
    for (int i = 0; i < THREADS; ++i) {
        if ((status = pthread_mutex_init(&subthread_lock_done[i], NULL))) {
            csa_error("failed to initialise 'subthread_lock_done[%i]' mutex: %s\n", i, strerror(status));
            destroy_mutexes(true, i, i);
            return -2;
        }
        if ((status = pthread_mutex_init(&subthread_lock_run[i],  NULL))) {
            csa_error("failed to initialise 'subthread_lock_run[%i]' mutex: %s\n", i, strerror(status));
            destroy_mutexes(true, i + 1, i);
            return -3;
        }
    }
    
    for(int t = 0; t < THREADS; ++t){
        pthread_mutex_lock(&subthread_lock_run[t]);
        if ((status = pthread_create(&threads[t], NULL, maintain_thread_generic, (void*)(intptr_t)t))) {
            csa_error("failed to create thread (thread index %i): %s\n", t, strerror(status));
            destroy_threads(t);
            destroy_mutexes(true, THREADS, THREADS);
            return -4;
        }
    }
#endif
    
    return 0;
}

int deinit_accelerate()
{
    int status = 0;
#if MULTITHREADING
    status += destroy_threads(THREADS);
    status += destroy_mutexes(true, THREADS, THREADS);
#endif
    return status;
}
