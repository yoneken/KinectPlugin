/*
  KinectImpl.h
  This is a sample program to use Kinect(TM) via Choreonoid.
  Author: Kenta Yonekura (yoneken)
*/  

#ifndef __KINECTIMPL_H__
#define __KINECTIMPL_H__

// Choose with your environment
//#define USE_OPENNI
#define USE_KINECT_SDK

#ifdef USE_KINECT_SDK
#include <ole2.h>
#include <NuiApi.h>
#endif /* USE_KINECT_SDK */

class KinectImpl
{
public:
	const static float Colors[][3];

	KinectImpl(void);
	~KinectImpl(void);

	virtual bool open(void);
	virtual void close(void);

	typedef enum _TEXTURE_INDEX{
		IMAGE_TEXTURE = 0,
		DEPTH_TEXTURE,
		TEXTURE_NUM,
	} TEXTURE_INDEX;

	virtual void initializeGL(void);
	virtual void drawTexture(int width, int height, TEXTURE_INDEX index);
	virtual void storeNuiDepth(void);
	virtual void storeNuiImage(void);
	virtual void storeNuiSkeleton(void);
	virtual void drawNuiSkeleton(int width, int height, int playerID);

	virtual void glDrawArrowd(double, double, double, double, double, double);
	virtual void drawHeadOrientation(int);

	virtual void setShowedTextureIndex(TEXTURE_INDEX index) { showedTextureIndex = index; }
	virtual int getShowedTextureIndex(void){ return showedTextureIndex; }
	virtual int getTrackedDataIndex(void){ return trackedDataIndex; };
#if defined(USE_AUDIO)
	virtual void initAudio(void);
	virtual void clearAudio(void);
	virtual void storeNuiAudio(void);
	virtual void drawSoundSource(int playerID);
#endif

protected:
	unsigned int bg_texture[TEXTURE_NUM];
	TEXTURE_INDEX showedTextureIndex;
	int trackedDataIndex;
	bool tracked;
	unsigned short depth[240][320];

#ifdef USE_KINECT_SDK
	INuiSensor *pNuiSensor;
	HANDLE hNextColorFrameEvent;
	HANDLE hNextDepthFrameEvent;
	HANDLE hNextSkeletonEvent;
	HANDLE pVideoStreamHandle;
	HANDLE pDepthStreamHandle;

	Vector4 skels[NUI_SKELETON_COUNT][NUI_SKELETON_POSITION_COUNT];
#endif /* USE_KINECT_SDK */

};

#endif /* __KINECTIMPL_H__ */