#ifndef CPT_SONAR_ASSIST_ACCELERATED_H
#define CPT_SONAR_ASSIST_ACCELERATED_H

#include "tracker.h"

#ifdef __cplusplus
extern "C" {
#endif

int bicubic(float *small, int s, float *big, int res, int offset);
void composite(Tracker *tracker, MapLayerColourMapping *mappings, int res, int stride, int layerCount, int layerSize);
void init_accelerate();

#ifdef __cplusplus
}
#endif

#endif
