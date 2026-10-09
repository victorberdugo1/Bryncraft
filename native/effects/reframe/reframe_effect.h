/*
 * reframe_effect.h — single-header "smart vertical crop" effect for raylib
 * Plain-C declarations above REFRAME_EFFECT_IMPLEMENTATION, C++ below
 *
 * Priority chain (director's eye):
 *   1. Face track  — Haar cascade + active-speaker mouth-motion score
 *   2. Body motion — foreground optical-flow cluster (person moving, no face)
 *   3. Golden/Fibonacci saliency — edge density weighted by φ focal points
 *
 * Part of Bryncraft (https://bryncraft.online/) — created by Victor Berdugo
 */

#ifndef REFRAME_EFFECT_H
#define REFRAME_EFFECT_H

#include "raylib.h"
#include "../../face_cascade_store.h"
#ifdef __EMSCRIPTEN__
#include "../../json_mini.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void ReframeEffect_Init(void);

#ifdef __EMSCRIPTEN__
void ReframeEffect_SetParams(const JsonValue *paramsObj);
#endif

void ReframeEffect_Update(float dt);
void ReframeEffect_Draw(RenderTexture2D scene, int screenW, int screenH);
void ReframeEffect_Unload(void);

#ifndef __EMSCRIPTEN__
bool RfCamera_Open(int deviceIndex);
bool RfCamera_IsOpen(void);
void RfCamera_CaptureInto(RenderTexture2D target);
void RfCamera_Close(void);
#endif

#ifdef __cplusplus
}
#endif

#endif

#if defined(REFRAME_EFFECT_IMPLEMENTATION) && !defined(REFRAME_EFFECT_IMPLEMENTATION_INCLUDED)
#define REFRAME_EFFECT_IMPLEMENTATION_INCLUDED

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/video/tracking.hpp>

#include <vector>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <numeric>

enum ReframeFallback {
	REFRAME_FALLBACK_CENTER = 0,
	REFRAME_FALLBACK_LAST_KNOWN,
};

struct ReframeParams {
	float targetAspectW, targetAspectH;
	float processScale;
	bool mirror;

	float faceScaleFactor;
	int faceMinNeighbors;
	float faceMinSizeFraction;

	float zoom;
	float headroom;
	float panSmoothing;
	float maxPanSpeed;
	float switchCooldown;
	float deadZone;
	bool activeSpeakerDetection;
	ReframeFallback fallbackMode;

	bool showDebugOverlay;
	Color debugBoxColor;
	Color debugTargetColor;
};

static ReframeParams g_rfParams = {
	.targetAspectW = 9.0f,
	.targetAspectH = 16.0f,
	.processScale = 0.75f,
	.mirror = false,

	.faceScaleFactor = 1.1f,
	.faceMinNeighbors = 3,
	.faceMinSizeFraction = 0.05f,

	.zoom = 1.0f,
	.headroom = 0.12f,
	.panSmoothing = 0.18f,
	.maxPanSpeed = 0.15f,
	.switchCooldown = 2.5f,
	.deadZone = 0.03f,
	.activeSpeakerDetection = true,
	.fallbackMode = REFRAME_FALLBACK_LAST_KNOWN,

	.showDebugOverlay = false,
	.debugBoxColor = (Color){ 120, 255, 120, 255 },
	.debugTargetColor = (Color){ 255, 200, 60, 255 },
};

struct ReframeTrack {
	int id;
	cv::Rect2f rect;
	float activity;
	int framesSinceSeen;
};

static std::vector<ReframeTrack> g_rfTracks;
static int g_rfNextTrackId = 1;
static int g_rfCurrentTargetId = -1;
static float g_rfTimeSinceSwitch = 1e9f;

static bool g_rfSmoothInit = false;
static float g_rfSmoothX = 0.5f;
static float g_rfSmoothY = 0.42f;
static float g_rfVelX = 0.0f;
static float g_rfVelY = 0.0f;
static float g_rfLastTargetX = 0.5f;
static float g_rfLastTargetY = 0.42f;
static int g_rfFramesSinceFace = 0;
extern int g_videoFramePushCount;
static int g_rfLastVideoPush = -1;
static float g_rfVideoDt = 1.0f / 30.0f;
static float g_rfDt = 1.0f / 60.0f;

static cv::CascadeClassifier g_rfFaceCascade;
static bool g_rfCascadeAttempted = false;
static bool g_rfCascadeOk = false;
static int g_rfCascadeLoadedVersion = -1;

static RenderTexture2D g_rfReadTarget;
static bool g_rfReadTargetReady = false;
static int g_rfReadW = 0, g_rfReadH = 0;

static cv::Mat g_rfPrevGray;
static cv::Mat g_rfMotionPrevGray;
static int g_rfFrameCounter = 0;

static float g_rfSaliencyX = 0.5f;
static float g_rfSaliencyY = 0.382f;
static int g_rfSaliencyTimer = 9999;

// Body-motion cluster (priority 2): normalized center of the dominant
// foreground motion region, used when faces are absent but a person moves.
static float g_rfMotionX = 0.5f;
static float g_rfMotionY = 0.5f;
static float g_rfMotionConfidence = 0.0f;
static bool g_rfMotionActive = false;

// φ — the four Fibonacci/golden-ratio focal points of a frame
static const float GX[4] = { 0.382f, 0.618f, 0.382f, 0.618f };
static const float GY[4] = { 0.382f, 0.382f, 0.618f, 0.618f };

// ---------------------------------------------------------------------------

static Color RfHexToColor(const char *hex, Color fallback) {
	if (!hex || hex[0] != '#') return fallback;
	size_t len = strlen(hex);
	unsigned int r, g, b;
	if (len >= 7) {
		if (sscanf(hex + 1, "%02x%02x%02x", &r, &g, &b) != 3) return fallback;
		return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 };
	}
	return fallback;
}

static ReframeFallback RfFallbackFromString(const char *s, ReframeFallback fallback) {
	if (!s) return fallback;
	if (strcmp(s, "center") == 0) return REFRAME_FALLBACK_CENTER;
	if (strcmp(s, "lastKnown") == 0) return REFRAME_FALLBACK_LAST_KNOWN;
	return fallback;
}

static float RfClamp(float v, float lo, float hi) {
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}

void ReframeEffect_Init(void) { }

#ifdef __EMSCRIPTEN__
void ReframeEffect_SetParams(const JsonValue *paramsObj) {
	if (!paramsObj) return;

	const char *aspectPreset = JsonAsString(JsonObjectGet(paramsObj, "aspectPreset"), NULL);
	if (aspectPreset) {
		float aw = g_rfParams.targetAspectW, ah = g_rfParams.targetAspectH;
		if (sscanf(aspectPreset, "%f:%f", &aw, &ah) == 2 && aw > 0.0f && ah > 0.0f) {
			g_rfParams.targetAspectW = aw;
			g_rfParams.targetAspectH = ah;
		}
	} else {
		g_rfParams.targetAspectW = (float)JsonAsNumber(JsonObjectGet(paramsObj, "targetAspectW"), g_rfParams.targetAspectW);
		g_rfParams.targetAspectH = (float)JsonAsNumber(JsonObjectGet(paramsObj, "targetAspectH"), g_rfParams.targetAspectH);
	}
	g_rfParams.processScale = (float)JsonAsNumber(JsonObjectGet(paramsObj, "processScale"), g_rfParams.processScale);
	g_rfParams.mirror = JsonAsBool(JsonObjectGet(paramsObj, "mirror"), g_rfParams.mirror);

	g_rfParams.faceScaleFactor = (float)JsonAsNumber(JsonObjectGet(paramsObj, "faceScaleFactor"), g_rfParams.faceScaleFactor);
	g_rfParams.faceMinNeighbors = (int)JsonAsNumber(JsonObjectGet(paramsObj, "faceMinNeighbors"), g_rfParams.faceMinNeighbors);
	g_rfParams.faceMinSizeFraction = (float)JsonAsNumber(JsonObjectGet(paramsObj, "faceMinSizeFraction"), g_rfParams.faceMinSizeFraction);

	g_rfParams.zoom = (float)JsonAsNumber(JsonObjectGet(paramsObj, "zoom"), g_rfParams.zoom);
	g_rfParams.headroom = (float)JsonAsNumber(JsonObjectGet(paramsObj, "headroom"), g_rfParams.headroom);
	g_rfParams.panSmoothing = (float)JsonAsNumber(JsonObjectGet(paramsObj, "panSmoothing"), g_rfParams.panSmoothing);
	g_rfParams.maxPanSpeed = (float)JsonAsNumber(JsonObjectGet(paramsObj, "maxPanSpeed"), g_rfParams.maxPanSpeed);
	g_rfParams.switchCooldown = (float)JsonAsNumber(JsonObjectGet(paramsObj, "switchCooldown"), g_rfParams.switchCooldown);
	g_rfParams.deadZone = (float)JsonAsNumber(JsonObjectGet(paramsObj, "deadZone"), g_rfParams.deadZone);
	g_rfParams.activeSpeakerDetection = JsonAsBool(JsonObjectGet(paramsObj, "activeSpeakerDetection"), g_rfParams.activeSpeakerDetection);
	g_rfParams.fallbackMode = RfFallbackFromString(JsonAsString(JsonObjectGet(paramsObj, "fallbackMode"), NULL), g_rfParams.fallbackMode);

	g_rfParams.showDebugOverlay = JsonAsBool(JsonObjectGet(paramsObj, "showDebugOverlay"), g_rfParams.showDebugOverlay);
	g_rfParams.debugBoxColor = RfHexToColor(JsonAsString(JsonObjectGet(paramsObj, "debugBoxColor"), NULL), g_rfParams.debugBoxColor);
	g_rfParams.debugTargetColor = RfHexToColor(JsonAsString(JsonObjectGet(paramsObj, "debugTargetColor"), NULL), g_rfParams.debugTargetColor);
}
#endif

void ReframeEffect_Update(float dt) {
	g_rfDt = dt > 0.0f ? dt : (1.0f / 60.0f);
}

static void RfEnsureCascadeLoaded() {
	const uint8_t *buf = NULL;
	size_t bufSize = 0;
	int version = -1;
	FaceCascadeStore_Get(&buf, &bufSize, &version);

	if (g_rfCascadeAttempted && version == g_rfCascadeLoadedVersion) return;
	g_rfCascadeAttempted = true;
	g_rfCascadeLoadedVersion = version;

	try {
		if (buf && bufSize > 0) {
			FILE *tmpFile = fopen("/tmp/reframe_cascade.xml", "wb");
			if (tmpFile) {
				fwrite(buf, 1, bufSize, tmpFile);
				fclose(tmpFile);
				g_rfCascadeOk = g_rfFaceCascade.load("/tmp/reframe_cascade.xml");
				if (!g_rfCascadeOk) {
					fprintf(stderr, "[reframe] Failed to load cascade from buffer (returned false)\n");
				}
			} else {
				fprintf(stderr, "[reframe] Failed to write cascade buffer to /tmp\n");
			}
		} else {
			fprintf(stderr, "[reframe] Cascade buffer not set. Call js_set_cascade_data() from JavaScript first.\n");
		}
	} catch (const cv::Exception &e) {
		fprintf(stderr, "[reframe] cv::Exception loading cascade: %s\n", e.what());
		g_rfCascadeOk = false;
	}
}

static void RfUpdateTracks(const std::vector<cv::Rect> &detections) {
	std::vector<bool> matched(detections.size(), false);

	for (auto &track : g_rfTracks) {
		int bestIdx = -1;
		float bestDist = 1e9f;
		for (size_t i = 0; i < detections.size(); i++) {
			if (matched[i]) continue;
			cv::Point2f detCenter(detections[i].x + detections[i].width * 0.5f, detections[i].y + detections[i].height * 0.5f);
			cv::Point2f trackCenter(track.rect.x + track.rect.width * 0.5f, track.rect.y + track.rect.height * 0.5f);
			float dist = cv::norm(detCenter - trackCenter);
			float threshold = std::max(track.rect.width, (float)detections[i].width) * 0.9f;
			if (dist < threshold && dist < bestDist) {
				bestDist = dist;
				bestIdx = (int)i;
			}
		}
		if (bestIdx >= 0) {
			matched[bestIdx] = true;
			const cv::Rect &d = detections[bestIdx];
			track.rect.x = track.rect.x * 0.5f + d.x * 0.5f;
			track.rect.y = track.rect.y * 0.5f + d.y * 0.5f;
			track.rect.width = track.rect.width * 0.5f + d.width * 0.5f;
			track.rect.height = track.rect.height * 0.5f + d.height * 0.5f;
			track.framesSinceSeen = 0;
		} else {
			track.framesSinceSeen++;
		}
	}

	for (size_t i = 0; i < detections.size(); i++) {
		if (matched[i]) continue;
		ReframeTrack t;
		t.id = g_rfNextTrackId++;
		t.rect = cv::Rect2f((float)detections[i].x, (float)detections[i].y, (float)detections[i].width, (float)detections[i].height);
		t.activity = 0.0f;
		t.framesSinceSeen = 0;
		g_rfTracks.push_back(t);
	}

	g_rfTracks.erase(std::remove_if(g_rfTracks.begin(), g_rfTracks.end(),
		[](const ReframeTrack &t) { return t.framesSinceSeen > 60; }), g_rfTracks.end());
}

static void RfUpdateActivity(const cv::Mat &gray) {
	if (g_rfPrevGray.empty() || g_rfPrevGray.size() != gray.size()) {
		g_rfPrevGray = gray.clone();
		return;
	}

	for (auto &track : g_rfTracks) {
		cv::Rect full(0, 0, gray.cols, gray.rows);
		cv::Rect mouthRoi(
			(int)(track.rect.x + track.rect.width * 0.2f),
			(int)(track.rect.y + track.rect.height * 0.55f),
			(int)(track.rect.width * 0.6f),
			(int)(track.rect.height * 0.4f));
		mouthRoi &= full;

		float score = 0.0f;
		if (mouthRoi.width > 2 && mouthRoi.height > 2) {
			cv::Mat diff;
			cv::absdiff(gray(mouthRoi), g_rfPrevGray(mouthRoi), diff);
			score = (float)(cv::mean(diff)[0] / 255.0);
		}
		track.activity = track.activity * 0.85f + score * 0.15f;
	}

	g_rfPrevGray = gray.clone();
}

static const ReframeTrack *RfFindTrack(int id) {
	for (const auto &t : g_rfTracks) if (t.id == id) return &t;
	return nullptr;
}

static void RfSelectTarget() {
	if (g_rfTracks.empty()) return;

	const ReframeTrack *best = &g_rfTracks[0];
	for (const auto &t : g_rfTracks) {
		bool better = g_rfParams.activeSpeakerDetection
			? (t.activity > best->activity)
			: (t.rect.area() > best->rect.area());
		if (&t != best && better) best = &t;
	}

	const ReframeTrack *current = RfFindTrack(g_rfCurrentTargetId);
	if (!current) {
		g_rfCurrentTargetId = best->id;
		g_rfTimeSinceSwitch = 0.0f;
		return;
	}
	if (current->id == best->id) return;

	bool marginCleared = g_rfParams.activeSpeakerDetection
		? (best->activity > current->activity + 0.015f)
		: (best->rect.area() > current->rect.area() * 1.2f);
	bool cooldownElapsed = g_rfTimeSinceSwitch >= g_rfParams.switchCooldown;

	if (marginCleared && cooldownElapsed) {
		g_rfCurrentTargetId = best->id;
		g_rfTimeSinceSwitch = 0.0f;
	}
}

static void RfComputeBodyMotion(const cv::Mat &gray) {
	if (g_rfMotionPrevGray.empty() || g_rfMotionPrevGray.size() != gray.size()) {
		g_rfMotionPrevGray = gray.clone();
		return;
	}

	cv::Mat prevForFlow = g_rfMotionPrevGray.clone();
	g_rfMotionPrevGray = gray.clone();

	std::vector<cv::Point2f> prevPts;
	cv::goodFeaturesToTrack(prevForFlow, prevPts, 300, 0.01, 5, cv::noArray(), 5);

	if (prevPts.size() < 8) {
		g_rfMotionConfidence *= 0.92f;
		return;
	}

	std::vector<cv::Point2f> nextPts;
	std::vector<uchar> flowStatus;
	std::vector<float> err;
	cv::calcOpticalFlowPyrLK(prevForFlow, gray, prevPts, nextPts, flowStatus, err,
		cv::Size(21, 21), 3,
		cv::TermCriteria(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 30, 0.01));

	std::vector<cv::Point2f> trackedPrev, trackedNext;
	for (size_t i = 0; i < prevPts.size(); i++) {
		if (flowStatus[i]) {
			trackedPrev.push_back(prevPts[i]);
			trackedNext.push_back(nextPts[i]);
		}
	}

	if (trackedPrev.size() < 8) {
		g_rfMotionConfidence *= 0.92f;
		return;
	}

	const float ransacThresh = 2.5f;
	const int ransacIter = 80;
	std::vector<uchar> inlierMask(trackedPrev.size(), 0);

	{
		int bestCount = -1;
		float bestTx = 0.0f, bestTy = 0.0f;
		int n = (int)trackedPrev.size();
		for (int iter = 0; iter < ransacIter; iter++) {
			int idx = rand() % n;
			float tx = trackedNext[idx].x - trackedPrev[idx].x;
			float ty = trackedNext[idx].y - trackedPrev[idx].y;
			int count = 0;
			for (int j = 0; j < n; j++) {
				float ex = (trackedNext[j].x - trackedPrev[j].x) - tx;
				float ey = (trackedNext[j].y - trackedPrev[j].y) - ty;
				if (sqrtf(ex * ex + ey * ey) < ransacThresh) count++;
			}
			if (count > bestCount) { bestCount = count; bestTx = tx; bestTy = ty; }
		}
		for (int j = 0; j < n; j++) {
			float ex = (trackedNext[j].x - trackedPrev[j].x) - bestTx;
			float ey = (trackedNext[j].y - trackedPrev[j].y) - bestTy;
			inlierMask[j] = (sqrtf(ex * ex + ey * ey) < ransacThresh) ? 1 : 0;
		}
	}

	const float W = (float)gray.cols;
	const float H = (float)gray.rows;
	const float minResidualMag = 1.5f;

	float sumX = 0.0f, sumY = 0.0f, sumMag = 0.0f;
	int foregroundMovers = 0;
	int totalTracked = (int)trackedPrev.size();

	for (size_t i = 0; i < trackedPrev.size(); i++) {
		bool isBackground = (!inlierMask.empty() && inlierMask[i]);
		if (isBackground) continue;

		float ny = trackedNext[i].y / H;
		if (ny > 0.88f) continue;

		float dx = trackedNext[i].x - trackedPrev[i].x;
		float dy = trackedNext[i].y - trackedPrev[i].y;
		float mag = sqrtf(dx * dx + dy * dy);
		if (mag < minResidualMag) continue;

		float nx = trackedNext[i].x / W;
		sumX += nx * mag;
		sumY += ny * mag;
		sumMag += mag;
		foregroundMovers++;
	}

	float fgRatio = (totalTracked > 0) ? ((float)foregroundMovers / (float)totalTracked) : 0.0f;
	float targetConfidence = RfClamp(fgRatio * 8.0f, 0.0f, 1.0f);
	g_rfMotionConfidence = g_rfMotionConfidence * 0.92f + targetConfidence * 0.08f;

	if (foregroundMovers >= 4 && sumMag > 0.0f) {
		float cx = sumX / sumMag;
		float cy = sumY / sumMag;
		g_rfMotionX = g_rfMotionX * 0.85f + cx * 0.15f;
		g_rfMotionY = g_rfMotionY * 0.85f + cy * 0.15f;
	}
}

// ---------------------------------------------------------------------------
// Priority 3: golden-ratio / Fibonacci saliency
//
// Sobel edge density in a 14×14 grid, weighted by Gaussian proximity to each
// of the four φ focal points. This is the cinematographer's rule-of-thirds
// taken to its mathematical ideal: the grid cell that has the most "content"
// (edges = visual complexity) AND sits nearest to where a director would
// place the subject wins. Result glides over time — same feel as a
// slow deliberate camera move between shots.
// ---------------------------------------------------------------------------
static void RfComputeGoldenSaliency(const cv::Mat &gray) {
	g_rfSaliencyTimer++;
	if (g_rfSaliencyTimer < 5) return;
	g_rfSaliencyTimer = 0;

	cv::Mat sx, sy, mag;
	cv::Sobel(gray, sx, CV_32F, 1, 0, 3);
	cv::Sobel(gray, sy, CV_32F, 0, 1, 3);
	cv::magnitude(sx, sy, mag);

	const int GRID = 14;
	float bw = (float)gray.cols / GRID;
	float bh = (float)gray.rows / GRID;

	float bestScore = -1.0f;
	float bestX = g_rfSaliencyX, bestY = g_rfSaliencyY;

	for (int r = 0; r < GRID; r++) {
		for (int c = 0; c < GRID; c++) {
			int x0 = (int)(c * bw), y0 = (int)(r * bh);
			int x1 = std::min((int)((c + 1) * bw), gray.cols);
			int y1 = std::min((int)((r + 1) * bh), gray.rows);
			if (x1 <= x0 || y1 <= y0) continue;

			float edgeDensity = (float)cv::mean(mag(cv::Rect(x0, y0, x1 - x0, y1 - y0)))[0] / 255.0f;

			float nx = (c + 0.5f) / GRID;
			float ny = (r + 0.5f) / GRID;

			float goldenAffinity = 0.0f;
			for (int i = 0; i < 4; i++) {
				float dx = nx - GX[i], dy = ny - GY[i];
				goldenAffinity += expf(-(dx * dx + dy * dy) * 20.0f);
			}
			goldenAffinity *= 0.25f;

			float score = edgeDensity * 0.55f + goldenAffinity * 0.45f;
			if (score > bestScore) {
				bestScore = score;
				bestX = nx;
				bestY = ny;
			}
		}
	}

	g_rfSaliencyX = g_rfSaliencyX * 0.72f + bestX * 0.28f;
	g_rfSaliencyY = g_rfSaliencyY * 0.72f + bestY * 0.28f;
}

void ReframeEffect_Draw(RenderTexture2D scene, int screenW, int screenH) {
	int frameW = scene.texture.width;
	int frameH = scene.texture.height;
	if (frameW <= 0 || frameH <= 0) return;

	float scale = RfClamp(g_rfParams.processScale, 0.1f, 1.0f);
	int workW = std::max(2, (int)(frameW * scale));
	int workH = std::max(2, (int)(frameH * scale));

	if (!g_rfReadTargetReady || workW != g_rfReadW || workH != g_rfReadH) {
		if (g_rfReadTargetReady) UnloadRenderTexture(g_rfReadTarget);
		g_rfReadTarget = LoadRenderTexture(workW, workH);
		g_rfReadTargetReady = true;
		g_rfReadW = workW;
		g_rfReadH = workH;
	}

	BeginTextureMode(g_rfReadTarget);
		ClearBackground(BLACK);
		DrawTexturePro(scene.texture,
			(Rectangle){ 0, 0, (float)frameW, -(float)frameH },
			(Rectangle){ 0, 0, (float)workW, (float)workH },
			(Vector2){ 0, 0 }, 0.0f, WHITE);
	EndTextureMode();

	Image img = LoadImageFromTexture(g_rfReadTarget.texture);
	cv::Mat rgba(workH, workW, CV_8UC4, img.data);
	cv::Mat work;
	cv::flip(rgba, work, 0);
	UnloadImage(img);
	if (g_rfParams.mirror) cv::flip(work, work, 1);

	cv::Mat gray;
	cv::cvtColor(work, gray, cv::COLOR_RGBA2GRAY);

	bool hasVideo = (g_videoFramePushCount > 0);
	bool newVideoFrame = !hasVideo || (g_videoFramePushCount != g_rfLastVideoPush);
	if (g_videoFramePushCount != g_rfLastVideoPush) {
		if (g_rfLastVideoPush >= 0)
			g_rfVideoDt = g_rfDt;
		g_rfLastVideoPush = g_videoFramePushCount;
	}
	if (!hasVideo) g_rfVideoDt = g_rfDt;

	RfEnsureCascadeLoaded();
	g_rfFrameCounter++;

	if (newVideoFrame) try {
		if (g_rfCascadeOk && g_rfFrameCounter % 3 == 0) {
			cv::Mat eq;
			cv::equalizeHist(gray, eq);
			int minSize = std::max(8, (int)(g_rfParams.faceMinSizeFraction * workW));
			std::vector<cv::Rect> faces;
			g_rfFaceCascade.detectMultiScale(eq, faces, g_rfParams.faceScaleFactor,
				g_rfParams.faceMinNeighbors, 0, cv::Size(minSize, minSize));
			RfUpdateTracks(faces);
		}
		RfUpdateActivity(gray);
		RfSelectTarget();
		RfComputeBodyMotion(gray);
		RfComputeGoldenSaliency(gray);
	} catch (const cv::Exception &e) {
		fprintf(stderr, "[reframe] cv::Exception: %s\n", e.what());
	}

	// -----------------------------------------------------------------------
	// Director's priority chain — pick the target point for this frame
	// -----------------------------------------------------------------------
	float desiredX = g_rfSmoothX, desiredY = g_rfSmoothY;

	const ReframeTrack *target = RfFindTrack(g_rfCurrentTargetId);

	if (target) {
		float rawX = (target->rect.x + target->rect.width * 0.5f) / workW;
		float rawY = (target->rect.y + target->rect.height * 0.5f) / workH - g_rfParams.headroom;
		desiredX = RfClamp(rawX, 0.0f, 1.0f);
		desiredY = RfClamp(rawY, 0.0f, 1.0f);
		g_rfLastTargetX = desiredX;
		g_rfLastTargetY = desiredY;
		g_rfFramesSinceFace = 0;
	} else if (++g_rfFramesSinceFace < 60) {
		desiredX = g_rfLastTargetX;
		desiredY = g_rfLastTargetY;
	} else if (g_rfMotionActive || g_rfMotionConfidence > 0.20f) {
		g_rfMotionActive = (g_rfMotionConfidence > 0.08f);
		desiredX = g_rfMotionX;
		desiredY = g_rfMotionY;
	} else if (g_rfParams.fallbackMode == REFRAME_FALLBACK_CENTER) {
		desiredX = 0.5f;
		desiredY = 0.42f;
	} else {
		desiredX = g_rfSaliencyX;
		desiredY = g_rfSaliencyY;
	}

	if (!g_rfSmoothInit) {
		g_rfSmoothX = desiredX;
		g_rfSmoothY = desiredY;
		g_rfVelX = 0.0f;
		g_rfVelY = 0.0f;
		g_rfSmoothInit = true;
	}

	if (newVideoFrame) {
		float vdt = g_rfVideoDt;
		float dx = desiredX - g_rfSmoothX;
		float dy = desiredY - g_rfSmoothY;
		if (fabsf(dx) < g_rfParams.deadZone) dx = 0.0f;
		if (fabsf(dy) < g_rfParams.deadZone) dy = 0.0f;

		float spring = RfClamp(g_rfParams.panSmoothing, 0.01f, 1.0f) * 4.0f;
		float damping = 2.0f * sqrtf(spring);
		g_rfVelX += (dx * spring - g_rfVelX * damping) * vdt;
		g_rfVelY += (dy * spring - g_rfVelY * damping) * vdt;

		float maxStep = g_rfParams.maxPanSpeed * vdt;
		g_rfVelX = RfClamp(g_rfVelX, -maxStep * 30.0f, maxStep * 30.0f);
		g_rfVelY = RfClamp(g_rfVelY, -maxStep * 30.0f, maxStep * 30.0f);

		float stepX = RfClamp(g_rfVelX * vdt, -maxStep, maxStep);
		float stepY = RfClamp(g_rfVelY * vdt, -maxStep, maxStep);
		g_rfSmoothX = RfClamp(g_rfSmoothX + stepX, 0.0f, 1.0f);
		g_rfSmoothY = RfClamp(g_rfSmoothY + stepY, 0.0f, 1.0f);

		g_rfTimeSinceSwitch += vdt;
	}

	float aspect = g_rfParams.targetAspectW / std::max(0.0001f, g_rfParams.targetAspectH);
	float zoom = std::max(1.0f, g_rfParams.zoom);
	float cropH = frameH / zoom;
	float cropW = cropH * aspect;
	if (cropW > frameW) { cropW = (float)frameW; cropH = cropW / aspect; }
	if (cropH > frameH) { cropH = (float)frameH; cropW = cropH * aspect; }

	float cx = g_rfSmoothX * frameW;
	float cy = g_rfSmoothY * frameH;
	float cropX = RfClamp(cx - cropW * 0.5f, 0.0f, std::max(0.0f, frameW - cropW));
	float cropY = RfClamp(cy - cropH * 0.5f, 0.0f, std::max(0.0f, frameH - cropH));

	float rawY = frameH - cropY - cropH;

	DrawTexturePro(scene.texture,
		(Rectangle){ cropX, rawY, cropW, -cropH },
		(Rectangle){ 0, 0, (float)screenW, (float)screenH },
		(Vector2){ 0, 0 }, 0.0f, WHITE);

	if (g_rfParams.showDebugOverlay) {
		for (const auto &t : g_rfTracks) {
			float faceCx = (t.rect.x + t.rect.width * 0.5f) / workW * frameW;
			float faceCy = (t.rect.y + t.rect.height * 0.5f) / workH * frameH;
			float faceW = t.rect.width / workW * frameW;
			float faceH = t.rect.height / workH * frameH;
			float dstX = (faceCx - faceW * 0.5f - cropX) * ((float)screenW / cropW);
			float dstY = (faceCy - faceH * 0.5f - cropY) * ((float)screenH / cropH);
			float dstW = faceW * ((float)screenW / cropW);
			float dstH = faceH * ((float)screenH / cropH);
			Color col = (t.id == g_rfCurrentTargetId) ? g_rfParams.debugTargetColor : g_rfParams.debugBoxColor;
			DrawRectangleLinesEx((Rectangle){ dstX, dstY, dstW, dstH }, 2.0f, col);
		}

		const char *modeStr =
			target             ? "P1:face"   :
			g_rfMotionConfidence > 0.15f ? "P2:motion" : "P3:golden";
		char label[96];
		snprintf(label, sizeof(label), "reframe: %d face(s)  conf=%.2f  [%s]",
			(int)g_rfTracks.size(), g_rfMotionConfidence, modeStr);
		DrawText(label, 8, 8, 18, g_rfParams.debugTargetColor);
	}
}

void ReframeEffect_Unload(void) {
	if (g_rfReadTargetReady) { UnloadRenderTexture(g_rfReadTarget); g_rfReadTargetReady = false; }
	g_rfPrevGray.release();
	g_rfMotionPrevGray.release();
	g_rfTracks.clear();
	g_rfCurrentTargetId = -1;
	g_rfSmoothInit = false;
	g_rfVelX = 0.0f;
	g_rfVelY = 0.0f;
	g_rfLastTargetX = 0.5f;
	g_rfLastTargetY = 0.42f;
	g_rfFramesSinceFace = 0;
	g_rfLastVideoPush = -1;
	g_rfVideoDt = 1.0f / 30.0f;
	g_rfSaliencyX = 0.5f;
	g_rfSaliencyY = 0.382f;
	g_rfSaliencyTimer = 9999;
	g_rfMotionX = 0.5f;
	g_rfMotionY = 0.5f;
	g_rfMotionConfidence = 0.0f;
	g_rfMotionActive = false;
}

#ifndef __EMSCRIPTEN__
#include <opencv2/videoio.hpp>

static cv::VideoCapture g_rfCamera;
static bool g_rfCameraOpen = false;
static Texture2D g_rfCameraTexture;
static bool g_rfCameraTextureReady = false;
static int g_rfCameraTexW = 0, g_rfCameraTexH = 0;

bool RfCamera_Open(int deviceIndex) {
	if (g_rfCameraOpen) return true;
	g_rfCameraOpen = g_rfCamera.open(deviceIndex);
	if (!g_rfCameraOpen) fprintf(stderr, "[reframe] Could not open camera %d\n", deviceIndex);
	return g_rfCameraOpen;
}

bool RfCamera_IsOpen(void) { return g_rfCameraOpen; }

void RfCamera_CaptureInto(RenderTexture2D target) {
	if (!g_rfCameraOpen) return;

	cv::Mat bgr;
	if (!g_rfCamera.read(bgr) || bgr.empty()) return;

	cv::Mat rgba;
	cv::cvtColor(bgr, rgba, cv::COLOR_BGR2RGBA);
	if (!rgba.isContinuous()) rgba = rgba.clone();

	if (!g_rfCameraTextureReady || g_rfCameraTexW != rgba.cols || g_rfCameraTexH != rgba.rows) {
		if (g_rfCameraTextureReady) UnloadTexture(g_rfCameraTexture);
		Image img = {
			.data = rgba.data,
			.width = rgba.cols,
			.height = rgba.rows,
			.mipmaps = 1,
			.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
		};
		g_rfCameraTexture = LoadTextureFromImage(img);
		g_rfCameraTexW = rgba.cols;
		g_rfCameraTexH = rgba.rows;
		g_rfCameraTextureReady = true;
	} else {
		UpdateTexture(g_rfCameraTexture, rgba.data);
	}

	BeginTextureMode(target);
		ClearBackground(BLACK);
		DrawTexturePro(g_rfCameraTexture,
			(Rectangle){ 0, 0, (float)g_rfCameraTexture.width, (float)g_rfCameraTexture.height },
			(Rectangle){ 0, 0, (float)target.texture.width, (float)target.texture.height },
			(Vector2){ 0, 0 }, 0.0f, WHITE);
	EndTextureMode();
}

void RfCamera_Close(void) {
	if (g_rfCameraOpen) { g_rfCamera.release(); g_rfCameraOpen = false; }
	if (g_rfCameraTextureReady) { UnloadTexture(g_rfCameraTexture); g_rfCameraTextureReady = false; }
	g_rfCameraTexW = 0;
	g_rfCameraTexH = 0;
}
#endif

#endif
