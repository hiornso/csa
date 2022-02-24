#include <gtk/gtk.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <semaphore.h>

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

#define USE_MULTITHREADING 1
#ifndef USE_VECTORS
#define USE_VECTORS 0
#endif

#define THREADS 8
#define VECSIZE 8
#define USE_LOADSTORE 0


#if USE_MULTITHREADING
#include <pthread.h>
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

static atomic_char32_t render_row;
static sem_t rendering_semaphore, start_semaphore, stop_semaphore;
static ThreadArg thread_arg;

static void launch_render()
{
    sem_wait(&rendering_semaphore); // wait for any other rendering processes to finish
    
    atomic_init(&render_row, 0);
    
    for(int i = 0; i < THREADS; ++i) {
        sem_post(&start_semaphore); // send a start rendering message to each thread
    }
    
    for(int i = 0; i < THREADS; ++i) {
        sem_wait(&stop_semaphore); // wait for all threads to finish rendering
    }
    
    sem_post(&rendering_semaphore); // mark that rendering is finished
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

static void bicubic_row(BicubicMatrix *matrices, int s, float *big, int res, int offset, int y)
{
    const int size = s + 2 * offset - 1;
    int row;
    float yy;
    if(offset){
        row = ((y + res/(2 * s)) * s) / res;
        yy = (( y - row * res / s + res/(2 * s) ) * s ) / (float)res;
    }else{
        row = (y * (s - 1)) / res;
        yy = ((s - 1) * y - row * res) / (float)res;
    }
    
    const float ycolvec[4] = {1.0f, yy, yy*yy, yy*yy*yy};
    float tmp[4];
    
    gsl_vector_float_const_view y_view = gsl_vector_float_const_view_array(ycolvec, 4);
    gsl_vector_float_view tmp_view = gsl_vector_float_view_array(tmp, 4);
    
    int lastcol = INT_MAX;
    
    for(int x = 0; x < res; ++x){
        
        int col;
        float xx;
        if(offset){
            col = ((x + res/(2 * s)) * s) / res;
            xx = (( x - col * res / s + res/(2 * s) ) * s ) / (float)res;
        }else{
            col = (x * (s - 1)) / res;
            xx = ((s - 1) * x - col * res) / (float)res;
        }
    
        if(lastcol != col) {
            gsl_matrix_float_const_view bicubic = gsl_matrix_float_const_view_array(matrices[row * size + col].mat, 4, 4);

            gsl_blas_sgemv(CblasNoTrans, 1.0f, &bicubic.matrix, &y_view.vector, 0.0f, &tmp_view.vector);
        }
        
        const float xrowvec[4] = {1.0f, xx, xx*xx, xx*xx*xx};
        
        gsl_matrix_float_const_view x_view = gsl_matrix_float_const_view_array(xrowvec, 1, 4);
        gsl_vector_float_view dest = gsl_vector_float_view_array(&big[y * res + x], 1);
        
        gsl_blas_sgemv(CblasNoTrans, 1.0f, &x_view.matrix, &tmp_view.vector, 1.0f, &dest.vector);
    }
}

#if USE_MULTITHREADING

static void maintain_thread_bicubic_row()
{
    ThreadArgBicubic ta = thread_arg.thread_arg.bicubic;
    int row;
    while((row = atomic_fetch_add_explicit(&render_row, 1, memory_order_relaxed)) < ta.res){
        bicubic_row(ta.matrices, ta.s, ta.big, ta.res, ta.offset, row);
    }
}

#endif

int32_t bicubic(float *small, int32_t s, float *big, int32_t res, int32_t offset)
{
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
#if USE_MULTITHREADING
    thread_arg.task = BICUBIC;
    thread_arg.thread_arg.bicubic = /*(ThreadArgBicubic)*/{
        matrices,
        big,
        s,
        res,
        offset
    };

    launch_render();
#else
    for(int y = 0; y < res; ++y){
        bicubic_row(matrices, s, big, res, offset, y);
    }
#endif

    csa_free(matrices);
	
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

static void maintain_thread_composite_row()
{
    ThreadArgComposite ta = thread_arg.thread_arg.composite;
    int row;
    while((row = atomic_fetch_add_explicit(&render_row, 1, memory_order_relaxed)) < ta.res){
        composite_row(ta.tracker, ta.mappings, ta.res, ta.stride, ta.layerCount, ta.layerSize, row);
    }
}

#endif

void composite(Tracker *tracker, MapLayerColourMapping *mappings, int res, int stride, int layerCount, int layerSize)
{
#if USE_MULTITHREADING
    thread_arg.task = COMPOSITE;
    thread_arg.thread_arg.composite = /*(ThreadArgComposite)*/{
        tracker,
        mappings,
        res,
        stride,
        layerCount,
        layerSize
    };
    
    launch_render();
#else
    for(int y = 0; y < res; ++y){
        composite_row(tracker, mappings, res, stride, layerCount, layerSize, y);
    }
#endif
}

#if USE_MULTITHREADING

__attribute__((noreturn)) static void* maintain_thread_generic(__attribute__((unused)) void *data)
{
    while(1){
        sem_wait(&start_semaphore); // wait to be told to start
        switch(thread_arg.task) {
            case BICUBIC:
                maintain_thread_bicubic_row();
                break;
            case COMPOSITE:
                maintain_thread_composite_row();
                break;
            default:
                csa_error("invalid value of 'thread_arg.task'. (Expected one of BICUBIC, COMPOSITE)\n");
        }
        sem_post(&stop_semaphore); // inform the main thread that rendering is done
    }
}

#endif

void init_accelerate()
{
#if OPENBLAS
    openblas_set_num_threads(1);
#endif

#if USE_MULTITHREADING
    sem_init(&rendering_semaphore, 0, 1);
    sem_init(&start_semaphore, 0, 0);
    sem_init(&stop_semaphore, 0, 0);
    
    pthread_t threads[THREADS];
    for(int t = 0; t < THREADS; ++t){
        pthread_create(threads + t, NULL, maintain_thread_generic, NULL);
    }
#endif
}
