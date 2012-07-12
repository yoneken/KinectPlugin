/*
  KinectView.cpp
  This is a sample program to use Kinect(TM) via Choreonoid.
  Author: Kenta Yonekura (yoneken)
*/  

#include "KinectView.h"
#include <cnoid/MessageView>
#include <QVBoxLayout>
#include <boost/bind.hpp>
#include "KinectImpl.h"

using namespace cnoid;


KinectScene::KinectScene(QWidget* parent)
    : QGLWidget(parent), tex_num(0), pKinectImpl(new KinectImpl())
{
	pKinectImpl->open();
}

KinectScene::~KinectScene(void)
{
	pKinectImpl->close();
}

void KinectScene::initializeGL()
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	pKinectImpl->initializeGL();

	connect( &timer, SIGNAL(timeout()), this, SLOT(idle()) );
	if(!timer.isActive()){
		timer.start(33);
		MessageView::mainInstance()->putln(tr("[Kinect] Initialize.\n"));
	}
}


void KinectScene::resizeGL(int width, int height)
{
	display_width = width;
	display_height = height;
}


void KinectScene::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, display_width, display_height);
	gluOrtho2D(0.0, (double)display_width, 0.0, (double)display_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	pKinectImpl->drawTexture(display_width, display_height, (KinectImpl::TEXTURE_INDEX)tex_num);
	pKinectImpl->drawNuiSkeleton(display_width, display_height, pKinectImpl->getTrackedDataIndex());
}

void KinectScene::mousePressEvent(QMouseEvent *event)
{
	if(tex_num == KinectImpl::IMAGE_TEXTURE){
		tex_num = KinectImpl::DEPTH_TEXTURE;
	}else{
		tex_num = KinectImpl::IMAGE_TEXTURE;
	}
}

void KinectScene::idle()
{
	pKinectImpl->storeNuiImage();
	pKinectImpl->storeNuiDepth();
	pKinectImpl->storeNuiSkeleton();
	update();
}

KinectView::KinectView() : kinectScene(false)
{
    setName("Kinect");

    kinectScene = new KinectScene(this);
    QVBoxLayout* vbox = new QVBoxLayout();
    vbox->addWidget(kinectScene);
    setLayout(vbox);
}


void KinectView::onActivated()
{
}


void KinectView::onDeactivated()
{
}
