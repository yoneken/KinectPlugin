#ifndef __FACE_TRACKER_H__
#define __FACE_TRACKER_H__

#define USE_KINECT_FACETRACKER

#define USE_KINECT_SDK
#ifdef USE_KINECT_SDK
#include <ole2.h>
#include <NuiApi.h>
#endif /* USE_KINECT_SDK */

void initFaceTracker(void);
void clearFaceTracker(void);
void storeFace(int playerID);

void setColorImage(unsigned char *, int);
void setDepthImage(unsigned char *, int);
void getRotation(float *, Vector4 (*skels)[NUI_SKELETON_POSITION_COUNT]);

#endif