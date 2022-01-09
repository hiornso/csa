#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <semaphore.h>

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

typedef struct cubic {
	float a,b,c,d;
} Cubic;

#if USE_MULTITHREADING

typedef struct thread_arg_bicubic {
    Cubic *cubics;
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

static Cubic cub_interp(float m, float n, float o, float p)
{ // https://dsp.stackexchange.com/questions/18265/bicubic-interpolation - BUT I COULD DO THE MATHS MYSELF IF I WANTED TO
	float a = 0.5f * (-m + 3.f * (n - o) + p);
	float b = m + 2.f * o - 0.5f * (5.f * n + p);
	float c = 0.5f * (o - m);
	float d = n;
	return /*(Cubic)*/{a,b,c,d};
}

static float cub_at_x(Cubic c, float x)
{
	float sq = x * x;
	float cu = sq * x;
	return c.a * cu + c.b * sq + c.c * x + c.d;
}

#if USE_VECTORS
static Vecf vec_cub_at_x(Vecf a, Vecf b, Vecf c, Vecf d, Vecf x)
{
    Vecf sq = x * x;
    Vecf cu = sq * x;
    return a * cu + b * sq + c * x + d;
}

static Vecf vec_cub_interp_at_x(Vecf m, Vecf n, Vecf o, Vecf p, Vecf x)
{
    Vecf a = 0.5f * (-m + 3.f * (n - o) + p);
	Vecf b = m + 2.f * o - 0.5f * (5.f * n + p);
	Vecf c = 0.5f * (o - m);
	Vecf d = n;
    
    return vec_cub_at_x(a,b,c,d, x);
}

#define INTERPOLATE_Y(DEST, IND) \
    ind_base = (IND) * sizeof(Cubic) / sizeof(float); /* index of cubic in cubics array * 4 floats per cubic */ \
    a = lookup<INT_MAX>(ind_base + offsetof(Cubic, a) / sizeof(float), (float const *)cubics); \
    b = lookup<INT_MAX>(ind_base + offsetof(Cubic, b) / sizeof(float), (float const *)cubics); \
    c = lookup<INT_MAX>(ind_base + offsetof(Cubic, c) / sizeof(float), (float const *)cubics); \
    d = lookup<INT_MAX>(ind_base + offsetof(Cubic, d) / sizeof(float), (float const *)cubics); \
    Vecf DEST = vec_cub_at_x(a,b,c,d, xx);

#endif

static void bicubic_row(Cubic *cubics, int s, float *big, int res, int offset, int y)
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
    
    int done_with_vectors = 0;
#if USE_VECTORS
    int iters = res / VECSIZE;
    done_with_vectors = iters * VECSIZE;
    
    Veci x = ASCENDING_VECTOR;
    for(int i = 0; i < iters; ++i){
        Veci col;
        Vecf xx;
        if(offset){
            col = ((x + res/(2 * s)) * s) / res;
            xx = to_float(( x - col * res / s + res/(2 * s) ) * s ) / (float)res;
        }else{
            col = (x * (s - 1)) / res;
            xx = to_float((s - 1) * x - col * res) / (float)res;
        }
        
        Vecf a,b,c,d;
        Veci ind_base;
        
        INTERPOLATE_Y(y_1,  CLAMP(row - offset - 1, 0, s - 1) * size + col);
        INTERPOLATE_Y(y0 ,  CLAMP(row - offset    , 0, s - 1) * size + col);
        INTERPOLATE_Y(y1 ,  CLAMP(row - offset + 1, 0, s - 1) * size + col);
        INTERPOLATE_Y(y2 ,  CLAMP(row - offset + 2, 0, s - 1) * size + col);
        
#if USE_LOADSTORE
        float *dest = big + y * res + i * VECSIZE;
        Vecf result;
        result.load(dest);
        result += vec_cub_interp_at_x(y_1, y0, y1, y2, yy);
        result.store(dest);
#else
        Vecf result = vec_cub_interp_at_x(y_1, y0, y1, y2, yy);
        for(int j = 0; j < VECSIZE; ++j){
            big[y * res + i * VECSIZE + j] += result[j];
        }
#endif
        x += VECSIZE;
    }
#endif
    for(int x = done_with_vectors; x < res; ++x){ // only need to do what has not been done already
        
        int col;
        float xx;
        if(offset){
            col = ((x + res/(2 * s)) * s) / res;
            xx = (( x - col * res / s + res/(2 * s) ) * s ) / (float)res;
        }else{
            col = (x * (s - 1)) / res;
            xx = ((s - 1) * x - col * res) / (float)res;
        }
        
        float y_1 = cub_at_x(cubics[CLAMP(row - offset - 1, 0, s - 1) * size + col], xx);
        float y0  = cub_at_x(cubics[CLAMP(row - offset    , 0, s - 1) * size + col], xx);
        float y1  = cub_at_x(cubics[CLAMP(row - offset + 1, 0, s - 1) * size + col], xx);
        float y2  = cub_at_x(cubics[CLAMP(row - offset + 2, 0, s - 1) * size + col], xx);
        
        Cubic c = cub_interp(y_1, y0, y1, y2);
        
        big[y * res + x] += cub_at_x(c, yy);
    }
}

#if USE_MULTITHREADING

static void maintain_thread_bicubic_row()
{
    ThreadArgBicubic ta = thread_arg.thread_arg.bicubic;
    int row;
    while((row = atomic_fetch_add_explicit(&render_row, 1, memory_order_relaxed)) < ta.res){
        bicubic_row(ta.cubics, ta.s, ta.big, ta.res, ta.offset, row);
    }
}

#endif

int bicubic(float *small, int s, float *big, int res, int offset)
{
    offset = offset ? 1 : 0;
    const int size = s + 2 * offset - 1;
    Cubic *cubics = (Cubic*)csa_malloc(sizeof(Cubic) * size * s); // could cache this, but probably not worth it - not a hot loop, and would increase memory usage a bit, so why bother
    if(cubics == NULL){
        return -1;
    }
    for(int row = 0; row < s; ++row){
        for(int col = 1 - offset; col < s + offset; ++col){
            cubics[row * size + col + offset - 1] = cub_interp(
                small[row * s + CLAMP(col - 2, 0, s - 1)],
                small[row * s + CLAMP(col - 1, 0, s - 1)],
                small[row * s + CLAMP(col    , 0, s - 1)],
                small[row * s + CLAMP(col + 1, 0, s - 1)]
            );
        }
    }
#if USE_MULTITHREADING
    thread_arg.task = BICUBIC;
    thread_arg.thread_arg.bicubic = /*(ThreadArgBicubic)*/{
        cubics,
        big,
        s,
        res,
        offset
    };

    launch_render();
#else
    for(int y = 0; y < res; ++y){
        bicubic_row(cubics, s, big, res, offset, y);
    }
#endif
    csa_free(cubics);
	
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
