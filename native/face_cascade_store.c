#include "face_cascade_store.h"

#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static uint8_t *g_faceCascadeStoreBuf = NULL;
static size_t g_faceCascadeStoreSize = 0;
static int g_faceCascadeStoreVersion = 0;

void FaceCascadeStore_Get(const uint8_t **outBuf, size_t *outSize, int *outVersion) {
	if (outBuf) *outBuf = g_faceCascadeStoreBuf;
	if (outSize) *outSize = g_faceCascadeStoreSize;
	if (outVersion) *outVersion = g_faceCascadeStoreVersion;
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void js_set_cascade_data(size_t bufSize, uint8_t *buf) {
	if (g_faceCascadeStoreBuf) free(g_faceCascadeStoreBuf);
	g_faceCascadeStoreBuf = NULL;
	g_faceCascadeStoreSize = 0;

	if (bufSize > 0 && buf) {
		g_faceCascadeStoreBuf = (uint8_t *)malloc(bufSize);
		if (g_faceCascadeStoreBuf) {
			memcpy(g_faceCascadeStoreBuf, buf, bufSize);
			g_faceCascadeStoreSize = bufSize;
			g_faceCascadeStoreVersion++;
		}
	}
}
