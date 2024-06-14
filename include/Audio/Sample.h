#ifndef _SAMPLE_H_
#define _SAMPLE_H_

#include "EstTypes.h"

#if __cplusplus
extern "C" {
#endif

EST_API EST_Sample *EST_SampleLoad(const char *filename);

EST_API EST_Sample *EST_SampleLoadFromMemory(const void *data, size_t size);

EST_API EST_RESULT EST_SampleFree(EST_Sample *sample);

#if __cplusplus
}
#endif

#endif