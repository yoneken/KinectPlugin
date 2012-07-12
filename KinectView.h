/*
  KinectView.h
  This is a sample program to use Kinect(TM) via Choreonoid.
  Author: Kenta Yonekura (yoneken)
*/  

#ifndef __KINECTVIEW_H__
#define __KINECTVIEW_H__

#include <cnoid/View>
#include <QGLWidget>
#include <QTimer>

class KinectImpl;

class KinectScene : public QGLWidget
{
	Q_OBJECT        // must include this if you use Qt signals/slots
public:
    KinectScene(QWidget* parent = 0);
    ~KinectScene();

protected:
	QTimer timer;
	int tex_num;
    virtual void initializeGL();
    virtual void resizeGL(int width, int height);
    virtual void paintGL();
	virtual void mousePressEvent(QMouseEvent *event);
    
protected Q_SLOTS:
	virtual void idle();
    
protected:
	int display_width, display_height;
	std::tr1::shared_ptr<KinectImpl> pKinectImpl;
};

    
class KinectView : public cnoid::View
{
public:
    KinectView();

protected:
    virtual void onActivated();
    virtual void onDeactivated();
    
private:
    KinectScene* kinectScene;
};

#endif /* __KINECTVIEW_H__ */