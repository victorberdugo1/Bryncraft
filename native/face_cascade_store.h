#ifndef FACE_CASCADE_STORE_H
#define FACE_CASCADE_STORE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Single owner of the Haar cascade XML buffer uploaded from JS via
// js_set_cascade_data() (see cascadeLoader.ts). Any effect that needs face
// detection (opencv_effect.h, reframe_effect.h, ...) reads it through
// FaceCascadeStore_Get() instead of declaring its own js_set_cascade_data —
// that symbol can only be defined once across the whole link, so it lives
// here, in face_cascade_store.c, and nowhere else.
//
// outVersion increments every time js_set_cascade_data() is called; a
// consumer compares it against the version it last loaded from to know
// whether it needs to reload its own cv::CascadeClassifier.
void FaceCascadeStore_Get(const uint8_t **outBuf, size_t *outSize, int *outVersion);

#ifdef __cplusplus
}
#endif

#endif // FACE_CASCADE_STORE_H
