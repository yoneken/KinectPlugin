/*
  KinectImpl.cpp
  This is a sample program to use Kinect(TM) via Choreonoid.
  Author: Kenta Yonekura (yoneken)
*/  

#include "KinectImpl.h"
#include <QGLWidget>
#include <cnoid/MessageView>
//#include "FaceTracker.h"

#ifdef USE_KINECT_SDK
#pragma comment(lib, "Kinect10.lib")
#endif /* USE_KINECT_SDK */

#define STDERR(str) cnoid::MessageView::mainInstance()->putln(str)
//#define STDERR(str) OutputDebugString(L##str)
//#define STDERR(str) 

const float KinectImpl::Colors[][3] ={{1,1,1},{0,1,1},{0,0,1},{0,1,0},{1,1,0},{1,0,0},{1,.5,0}};	// Order is Blue-Green-Red


KinectImpl::KinectImpl(void)
	:showedTextureIndex(IMAGE_TEXTURE), trackedDataIndex(0), tracked(false)
{
}


KinectImpl::~KinectImpl(void)
{
	for(int i=0;i<TEXTURE_NUM;i++){
		glDeleteTextures(1, &bg_texture[i]);
	}
}

bool KinectImpl::open(void)
{
	HRESULT hr;

	hr = NuiCreateSensorByIndex(0, &pNuiSensor);
	if(FAILED(hr)){
		STDERR("Cannot connect with kinect0.\r\n");
		return false;
	}

	hr = pNuiSensor->NuiInitialize(
		//NUI_INITIALIZE_FLAG_USES_DEPTH |
		NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | 
		NUI_INITIALIZE_FLAG_USES_COLOR | 
		NUI_INITIALIZE_FLAG_USES_SKELETON
		);
	if ( E_NUI_SKELETAL_ENGINE_BUSY == hr ){
		hr = pNuiSensor->NuiInitialize(
			NUI_INITIALIZE_FLAG_USES_DEPTH |
			NUI_INITIALIZE_FLAG_USES_COLOR
			);
	}
	if(FAILED(hr)){
		STDERR("Cannot initialize kinect.\r\n");
		return false;
	}

	hNextColorFrameEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	hNextDepthFrameEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	hNextSkeletonEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

	if(HasSkeletalEngine(pNuiSensor)){
		hr = pNuiSensor->NuiSkeletonTrackingEnable( hNextSkeletonEvent, 
			//NUI_SKELETON_TRACKING_FLAG_TITLE_SETS_TRACKED_SKELETONS |
			//NUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORT 
			0
			);
		if(FAILED(hr)){
			STDERR("Cannot track skeletons\r\n");
			return false;
		}
	}

	hr = pNuiSensor->NuiImageStreamOpen(
		NUI_IMAGE_TYPE_COLOR,
		NUI_IMAGE_RESOLUTION_640x480,
		0,
		2,
		hNextColorFrameEvent,
		&pVideoStreamHandle );
	if(FAILED(hr)){
		STDERR("Cannot open image stream\r\n");
		return false;
	}

	hr = pNuiSensor->NuiImageStreamOpen(
		HasSkeletalEngine(pNuiSensor) ? NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX : NUI_IMAGE_TYPE_DEPTH,
		NUI_IMAGE_RESOLUTION_320x240,
		0,
		2,
		hNextDepthFrameEvent,
		&pDepthStreamHandle );
	if(FAILED(hr)){
		STDERR("Cannot open depth and player stream\r\n");
		return false;
	}
/*
	hr = pNuiSensor->NuiImageStreamOpen(
		NUI_IMAGE_TYPE_DEPTH,
		NUI_IMAGE_RESOLUTION_640x480,
		0,
		2,
		hNextDepthFrameEvent,
		&pDepthStreamHandle );
	if(FAILED(hr)){
		STDERR("Cannot open depth stream\r\n");
		return false;
	}
*/
#if defined(USE_KINECT_FACETRACKER)
	initFaceTracker();
#endif
	return true;
}

void KinectImpl::close(void)
{
	CloseHandle( hNextColorFrameEvent );
	hNextColorFrameEvent = NULL;

	CloseHandle( hNextDepthFrameEvent );
	hNextDepthFrameEvent = NULL;

	CloseHandle( hNextSkeletonEvent );
	hNextSkeletonEvent = NULL;
	if(HasSkeletalEngine(pNuiSensor)) pNuiSensor->NuiSkeletonTrackingDisable();

#if defined(USE_KINECT_FACETRACKER)
	clearFaceTracker();
#endif

	pNuiSensor->NuiShutdown();
	pNuiSensor->Release();
	pNuiSensor = NULL;
}

void KinectImpl::initializeGL(void)
{
	for(int i=0;i<TEXTURE_NUM;i++){
		glGenTextures(1, &bg_texture[i]);
		glBindTexture(GL_TEXTURE_2D, bg_texture[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	}
}

void KinectImpl::drawTexture(int width, int height, TEXTURE_INDEX index = (TEXTURE_INDEX)99)
{
	short vertices[] = {
		-1,	-1,
		 1,	-1,
		-1,	 1,
		 1,	 1,
	};
	GLfloat texCoords[] = {
		1.0f,	1.0f,
		0.0f,	1.0f,
		1.0f,	0.0f,
		0.0f,	0.0f
	};
	
	if(index >= TEXTURE_NUM) index = showedTextureIndex;
	else{
		if(index != showedTextureIndex) showedTextureIndex = index;
	}

	glDisable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_FLAT);
	glDisableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_SHORT, 0, vertices);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, texCoords);

	glBindTexture(GL_TEXTURE_2D, bg_texture[index]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_TEXTURE_2D);
}

void KinectImpl::storeNuiDepth(void)
{
	NUI_IMAGE_FRAME depthFrame;

	if(WAIT_OBJECT_0 != WaitForSingleObject(hNextDepthFrameEvent, 0)) return;

	HRESULT hr = pNuiSensor->NuiImageStreamGetNextFrame(
		pDepthStreamHandle,
		0,
		&depthFrame );
	if( FAILED( hr ) ){
		return;
	}
	if(depthFrame.eImageType != NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX)
		STDERR("Depth type is not match with the depth and players\r\n");

	INuiFrameTexture *pTexture = depthFrame.pFrameTexture;
	NUI_LOCKED_RECT LockedRect;
	pTexture->LockRect( 0, &LockedRect, NULL, 0 );
	if( LockedRect.Pitch != 0 ){
		NUI_SURFACE_DESC pDesc;
#if defined(USE_KINECT_FACETRACKER)
		setDepthImage(LockedRect.pBits, LockedRect.size);
#endif
		pTexture->GetLevelDesc(0, &pDesc);
		//printf("w: %d, h: %d, byte/pixel: %d\r\n", pDesc.Width, pDesc.Height, LockedRect.Pitch/pDesc.Width);

		unsigned short *pBuffer = (unsigned short *)LockedRect.pBits;
		memcpy(depth, LockedRect.pBits, pTexture->BufferLen());

		unsigned short *p = (unsigned short *)pBuffer;
		for(int i=0;i<pTexture->BufferLen()/2;i++){
			//*p = (unsigned short)((*p & 0xff00)>>8) | ((*p & 0x00ff)<<8);	// for check
			*p = NuiDepthPixelToDepth(*p);
			p++;
		}
		glBindTexture(GL_TEXTURE_2D, bg_texture[DEPTH_TEXTURE]);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,
			pDesc.Width,  pDesc.Height,
			0, GL_LUMINANCE, GL_UNSIGNED_SHORT, pBuffer);
		pTexture->UnlockRect(0);
	}
	else{
		STDERR("Buffer length of received texture is bogus\r\n");
	}
	pNuiSensor->NuiImageStreamReleaseFrame( pDepthStreamHandle, &depthFrame );
}

void KinectImpl::storeNuiImage(void)
{
	NUI_IMAGE_FRAME imageFrame;

	if(WAIT_OBJECT_0 != WaitForSingleObject(hNextColorFrameEvent, 0)) return;

	HRESULT hr =  pNuiSensor->NuiImageStreamGetNextFrame(
		pVideoStreamHandle,
		0,
		&imageFrame );
	if( FAILED( hr ) ){
		return;
	}
	if(imageFrame.eImageType != NUI_IMAGE_TYPE_COLOR)
		STDERR("Image type is not match with the color\r\n");

	INuiFrameTexture *pTexture = imageFrame.pFrameTexture;
	NUI_LOCKED_RECT LockedRect;
	pTexture->LockRect( 0, &LockedRect, NULL, 0 );
	if( LockedRect.Pitch != 0 ){
		byte *pBuffer = (byte *)LockedRect.pBits;
#if defined(USE_KINECT_FACETRACKER)
		setColorImage(LockedRect.pBits, LockedRect.size);
#endif
		NUI_SURFACE_DESC pDesc;
		pTexture->GetLevelDesc(0, &pDesc);
		//printf("w: %d, h: %d, byte/pixel: %d\r\n", pDesc.Width, pDesc.Height, LockedRect.Pitch/pDesc.Width);
		typedef struct t_RGBA{
			byte r;
			byte g;
			byte b;
			byte a;
		};
		t_RGBA *p = (t_RGBA *)pBuffer;
		for(int i=0;i<pTexture->BufferLen()/4;i++){
			byte b = p->b;
			p->b = p->r;
			p->r = b;
			p->a = (byte)255;
			p++;
		}

		glBindTexture(GL_TEXTURE_2D, bg_texture[IMAGE_TEXTURE]);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
			pDesc.Width,  pDesc.Height,
			0, GL_RGBA, GL_UNSIGNED_BYTE, pBuffer);
		pTexture->UnlockRect(0);
	}else{
		STDERR("Buffer length of received texture is bogus\r\n");
	}

	pNuiSensor->NuiImageStreamReleaseFrame( pVideoStreamHandle, &imageFrame );
}

void KinectImpl::storeNuiSkeleton(void)
{
	if(WAIT_OBJECT_0 != WaitForSingleObject(hNextSkeletonEvent, 0)) return;

	NUI_SKELETON_FRAME SkeletonFrame = {0};
	HRESULT hr = pNuiSensor->NuiSkeletonGetNextFrame( 0, &SkeletonFrame );

	bool bFoundSkeleton = false;
	for( int i = 0 ; i < NUI_SKELETON_COUNT ; i++ ){
		if( SkeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED ){
			bFoundSkeleton = true;
			trackedDataIndex = i;
		}
	}

	tracked = bFoundSkeleton;
	// no skeletons!
	if( !bFoundSkeleton )
		return;

	// smooth out the skeleton data
	pNuiSensor->NuiTransformSmooth(&SkeletonFrame,NULL);

	// store each skeleton color according to the slot within they are found.
	for( int i = 0 ; i < NUI_SKELETON_COUNT ; i++ )
	{
		if( (SkeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED)){
			memcpy(skels[i], SkeletonFrame.SkeletonData[i].SkeletonPositions, sizeof(Vector4)*NUI_SKELETON_POSITION_COUNT);
		}
	}
}

void KinectImpl::drawNuiSkeleton(int width, int height, int playerID)
{
	int scaleX = width;
	int scaleY = height;
	long x=0,y=0;
	unsigned short depth=0;
	float fx=0,fy=0;
	long cx=0,cy=0;
	float display_posf[NUI_SKELETON_POSITION_COUNT][2];

	for (int i = 0; i < NUI_SKELETON_POSITION_COUNT; i++)
	{
		switch(showedTextureIndex){
			case IMAGE_TEXTURE:
				NuiTransformSkeletonToDepthImage( skels[playerID][i], &x, &y, &depth);
				NuiImageGetColorPixelCoordinatesFromDepthPixel(NUI_IMAGE_RESOLUTION_640x480, NULL, x, y, depth, &cx, &cy);
				display_posf[i][0] = 1.0f - (cx / 640.0f) * 2.0f;
				display_posf[i][1] = 1.0f - (cy / 480.0f) * 2.0f;
				break;
			case DEPTH_TEXTURE:
				NuiTransformSkeletonToDepthImage( skels[playerID][i], &fx, &fy);
				display_posf[i][0] = 1.0f - fx / 320.0f * 2.0f;
				display_posf[i][1] = 1.0f - fy / 240.0f * 2.0f;
				break;
			default:
				break;
		}
	}

	glColor3ub(255, 255, 0);
	glLineWidth(6);
	glBegin(GL_LINE_STRIP);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_HIP_CENTER][0], display_posf[NUI_SKELETON_POSITION_HIP_CENTER][1]);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_SPINE][0], display_posf[NUI_SKELETON_POSITION_SPINE][1]);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_SHOULDER_CENTER][0], display_posf[NUI_SKELETON_POSITION_SHOULDER_CENTER][1]);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_HEAD][0], display_posf[NUI_SKELETON_POSITION_HEAD][1]);
	glEnd();
	glBegin(GL_LINE_STRIP);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_SHOULDER_CENTER][0], display_posf[NUI_SKELETON_POSITION_SHOULDER_CENTER][1]);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_SHOULDER_LEFT][0], display_posf[NUI_SKELETON_POSITION_SHOULDER_LEFT][1]);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_ELBOW_LEFT][0], display_posf[NUI_SKELETON_POSITION_ELBOW_LEFT][1]);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_WRIST_LEFT][0], display_posf[NUI_SKELETON_POSITION_WRIST_LEFT][1]);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_HAND_LEFT][0], display_posf[NUI_SKELETON_POSITION_HAND_LEFT][1]);
	glEnd();
	glBegin(GL_LINE_STRIP);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_SHOULDER_CENTER][0], display_posf[NUI_SKELETON_POSITION_SHOULDER_CENTER][1]);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_SHOULDER_RIGHT][0], display_posf[NUI_SKELETON_POSITION_SHOULDER_RIGHT][1]);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_ELBOW_RIGHT][0], display_posf[NUI_SKELETON_POSITION_ELBOW_RIGHT][1]);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_WRIST_RIGHT][0], display_posf[NUI_SKELETON_POSITION_WRIST_RIGHT][1]);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_HAND_RIGHT][0], display_posf[NUI_SKELETON_POSITION_HAND_RIGHT][1]);
	glEnd();
	glBegin(GL_LINE_STRIP);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_HIP_CENTER][0], display_posf[NUI_SKELETON_POSITION_HIP_CENTER][1]);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_HIP_LEFT][0], display_posf[NUI_SKELETON_POSITION_HIP_LEFT][1]);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_KNEE_LEFT][0], display_posf[NUI_SKELETON_POSITION_KNEE_LEFT][1]);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_ANKLE_LEFT][0], display_posf[NUI_SKELETON_POSITION_ANKLE_LEFT][1]);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_FOOT_LEFT][0], display_posf[NUI_SKELETON_POSITION_FOOT_LEFT][1]);
	glEnd();
	glBegin(GL_LINE_STRIP);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_HIP_CENTER][0], display_posf[NUI_SKELETON_POSITION_HIP_CENTER][1]);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_HIP_RIGHT][0], display_posf[NUI_SKELETON_POSITION_HIP_RIGHT][1]);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_KNEE_RIGHT][0], display_posf[NUI_SKELETON_POSITION_KNEE_RIGHT][1]);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_ANKLE_RIGHT][0], display_posf[NUI_SKELETON_POSITION_ANKLE_RIGHT][1]);
		glVertex2f( display_posf[NUI_SKELETON_POSITION_FOOT_RIGHT][0], display_posf[NUI_SKELETON_POSITION_FOOT_RIGHT][1]);
	glEnd();
	glColor3ub(0, 0, 0);
}

void KinectImpl::glDrawArrowd(double x0, double y0, double z0,
 double x1, double y1, double z1)
{
	GLUquadricObj *arrows[2];
	double x2, y2, z2, len, ang;

	x2 = x1-x0; y2 = y1-y0; z2 = z1-z0;
	len = sqrt(x2*x2 + y2*y2 + z2*z2);
	if(len != 0.0){
		ang = acos(z2*len/(sqrt(x2*x2+y2*y2+z2*z2)*len))/3.1415926*180.0;

		glPushMatrix();
			glTranslated( x0, y0, z0);
			glRotated( ang, -y2*len, x2*len, 0.0);
			arrows[0] = gluNewQuadric();
			gluQuadricDrawStyle(arrows[0], GLU_FILL);
			gluCylinder(arrows[0], len/80, len/80, len*0.9, 8, 8);
			glPushMatrix();
				glTranslated( 0.0, 0.0, len*0.9);
				arrows[1] = gluNewQuadric();
				gluQuadricDrawStyle(arrows[1], GLU_FILL);
				gluCylinder(arrows[1], len/30, 0.0f, len/10, 8, 8);
			glPopMatrix();
		glPopMatrix();
	}
}

void KinectImpl::drawHeadOrientation(int playerID)
{
	float vHead[3];
	//getRotation(vHead);

	glColor3ub(255, 0, 0);
	glDrawArrowd(skels[playerID][NUI_SKELETON_POSITION_HEAD].z, -skels[playerID][NUI_SKELETON_POSITION_HEAD].x, 
			skels[playerID][NUI_SKELETON_POSITION_HEAD].y, skels[playerID][NUI_SKELETON_POSITION_HEAD].z + vHead[2]*0.01,
			-skels[playerID][NUI_SKELETON_POSITION_HEAD].x - vHead[0]*0.01, skels[playerID][NUI_SKELETON_POSITION_HEAD].y + vHead[1]*0.01);
	glColor3ub(0, 0, 0);
}

#if defined(USE_AUDIO)
const int AudioSamplesPerEnergySample = 40;
INuiAudioBeam* pNuiAudioSource = NULL;
IMediaObject* pDMO = NULL;
IPropertyStore* pPropertyStore = NULL;
float accumulatedSquareSum = 0.0f;
int accumulatedSampleCount = 0;
double beamAngle, sourceAngle, sourceConfidence;

class CStaticMediaBuffer : public IMediaBuffer
{
public:
    CStaticMediaBuffer() : m_dataLength(0) {}

    // IUnknown methods
    STDMETHODIMP_(ULONG) AddRef() { return 2; }
    STDMETHODIMP_(ULONG) Release() { return 1; }
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
    {
        if (riid == IID_IUnknown)
        {
            AddRef();
            *ppv = (IUnknown*)this;
            return NOERROR;
        }
        else if (riid == IID_IMediaBuffer)
        {
            AddRef();
            *ppv = (IMediaBuffer*)this;
            return NOERROR;
        }
        else
        {
            return E_NOINTERFACE;
        }
    }

    STDMETHODIMP SetLength(DWORD length) {m_dataLength = length; return NOERROR;}
    STDMETHODIMP GetMaxLength(DWORD *pMaxLength) {*pMaxLength = sizeof(m_pData); return NOERROR;}
    STDMETHODIMP GetBufferAndLength(BYTE **ppBuffer, DWORD *pLength)
    {
        if (ppBuffer)
        {
            *ppBuffer = m_pData;
        }
        if (pLength)
        {
            *pLength = m_dataLength;
        }
        return NOERROR;
    }
    void Init(ULONG ulData)
    {
        m_dataLength = ulData;
    }

protected:
    BYTE m_pData[16000 * 2];
    ULONG m_dataLength;
};
CStaticMediaBuffer      captureBuffer;

void KinectImpl::initAudio(void)
{
	HRESULT hr = pNuiSensor->NuiGetAudioSource(&pNuiAudioSource);
	if(FAILED(hr)){
		 STDERR("Cannot open audio stream.\r\n");
	}
	hr = pNuiAudioSource->QueryInterface(IID_IMediaObject, (void**)&pDMO);
	if(FAILED(hr)){
		 STDERR("Cannot get media object.\r\n");
	}
	hr = pNuiAudioSource->QueryInterface(IID_IPropertyStore, (void**)&pPropertyStore);
	if(FAILED(hr)){
		 STDERR("Cannot get media property.\r\n");
	}

	PROPVARIANT pvSysMode;
	PropVariantInit(&pvSysMode);
	pvSysMode.vt = VT_I4;
	pvSysMode.lVal = (LONG)(2);
	pPropertyStore->SetValue(MFPKEY_WMAAECMA_SYSTEM_MODE, pvSysMode);
	PropVariantClear(&pvSysMode);

	WAVEFORMATEX wfxOut = {WAVE_FORMAT_PCM, 1, 16000, 32000, 2, 16, 0};
	DMO_MEDIA_TYPE mt = {0};
	MoInitMediaType(&mt, sizeof(WAVEFORMATEX));
    
	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = MEDIASUBTYPE_PCM;
	mt.lSampleSize = 0;
	mt.bFixedSizeSamples = TRUE;
	mt.bTemporalCompression = FALSE;
	mt.formattype = FORMAT_WaveFormatEx;	
	memcpy(mt.pbFormat, &wfxOut, sizeof(WAVEFORMATEX));
    
	hr = pDMO->SetOutputType(0, &mt, 0); 
	MoFreeMediaType(&mt);
}

void KinectImpl::clearAudio(void)
{
	pNuiAudioSource->Release();
	pNuiAudioSource = NULL;
	pDMO->Release();
	pDMO = NULL;
	pPropertyStore->Release();
	pPropertyStore = NULL;
}

void KinectImpl::storeNuiAudio(void)
{
	HRESULT hr;
	DWORD dwStatus = 0;
	DMO_OUTPUT_DATA_BUFFER outputBuffer = {0};
	outputBuffer.pBuffer = &captureBuffer;

	captureBuffer.Init(0);
	hr = pDMO->ProcessOutput(0, 1, &outputBuffer, &dwStatus);
	if(FAILED(hr)) STDERR("Failed to process audio output.\r\n");

	if(hr != S_FALSE){
		unsigned char *pProduced = NULL;
		unsigned long cbProduced = 0;
		captureBuffer.GetBufferAndLength(&pProduced, &cbProduced);

		float maxVolume = 0.0f;
		if (cbProduced > 0){
			for (unsigned int i = 0; i < cbProduced; i += 2){
				short audioSample = static_cast<short>(pProduced[i] | (pProduced[i+1] << 8));
				accumulatedSquareSum += audioSample * audioSample;
				++accumulatedSampleCount;
				if (accumulatedSampleCount < AudioSamplesPerEnergySample) continue;

				float meanSquare = accumulatedSquareSum / AudioSamplesPerEnergySample;
				float amplitude = log(meanSquare) / log(static_cast<float>(INT_MAX));

				if(amplitude > maxVolume) maxVolume = amplitude;

				accumulatedSquareSum = 0;
				accumulatedSampleCount = 0;
			}
		}

		if(maxVolume > 0.7){
			//printf("%.2f\r\n", maxVolume);
			pNuiAudioSource->GetBeam(&beamAngle);
			pNuiAudioSource->GetPosition(&sourceAngle, &sourceConfidence);
			//printf("%.2f\t%.1f\r\n", sourceAngle, sourceConfidence);
		}else{
			sourceConfidence = 0.0;
		}
	}
}

void KinectImpl::drawSoundSource(int playerID)
{
	const float allowErrAngle = 0.2;
	const int JointsNum = 5;
	int searchJointArray[JointsNum] = {NUI_SKELETON_POSITION_HEAD,NUI_SKELETON_POSITION_HAND_RIGHT,NUI_SKELETON_POSITION_HAND_LEFT,NUI_SKELETON_POSITION_FOOT_RIGHT,NUI_SKELETON_POSITION_FOOT_LEFT};
	float skelAngles[JointsNum];
	float mostNearAngle = 3.14;
	int mostNearJoint;
	static long cx=0,cy=0;

	if(sourceConfidence > 0.3){
		//printf("x : %.2f\ty : %.2f\tz : %.2f\r\n", skels[playerID][NUI_SKELETON_POSITION_HAND_RIGHT].x, skels[playerID][NUI_SKELETON_POSITION_HAND_RIGHT].y, skels[playerID][NUI_SKELETON_POSITION_WRIST_RIGHT].z);
		//printf("%.2f\r\n", atan2(skels[playerID][NUI_SKELETON_POSITION_WRIST_RIGHT].x, skels[playerID][NUI_SKELETON_POSITION_WRIST_RIGHT].z));

		for(int i=0;i<5;i++){
			skelAngles[i] = atan2f(skels[playerID][searchJointArray[i]].x, skels[playerID][searchJointArray[i]].z);
			float subAngle = abs(sourceAngle - skelAngles[i]);
			if(mostNearAngle > subAngle){
				mostNearAngle = subAngle;
				mostNearJoint = i;
			}
		}

		if(mostNearAngle < allowErrAngle){
			long x=0,y=0;
			unsigned short depth=0;

			NuiTransformSkeletonToDepthImage( skels[playerID][searchJointArray[mostNearJoint]], &x, &y, &depth);
			NuiImageGetColorPixelCoordinatesFromDepthPixel(NUI_IMAGE_RESOLUTION_640x480, NULL, x, y, depth, &cx, &cy);
		}
	}

	// draw a square;
	glColor3ub(255, 0, 0);
	glLineWidth(3);
	glBegin(GL_LINE_LOOP);
		glVertex2i( DEFAULT_WIDTH - cx - 10, DEFAULT_HEIGHT - cy - 10);
		glVertex2i( DEFAULT_WIDTH - cx - 10, DEFAULT_HEIGHT - cy + 10);
		glVertex2i( DEFAULT_WIDTH - cx + 10, DEFAULT_HEIGHT - cy + 10);
		glVertex2i( DEFAULT_WIDTH - cx + 10, DEFAULT_HEIGHT - cy - 10);
	glEnd();
	glColor3ub(0, 0, 0);
}
#endif