#ifndef CPT_SONAR_ASSIST_ACCELERATED_H
#define CPT_SONAR_ASSIST_ACCELERATED_H

#include "tracker.h"

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t bicubic(float *small, int32_t s, float *big, int32_t res, int32_t offset);
void composite(Tracker *tracker, MapLayerColourMapping *mappings, int res, int stride, int layerCount, int layerSize);
int init_accelerate();

#ifdef __cplusplus
}
#endif

#endif
