
#include "KinectView.h"
#include <cnoid/Plugin>

using namespace cnoid;

class KinectPlugin : public Plugin
{
public:
    
    KinectPlugin() : Plugin("Kinect") { }
    
    virtual bool initialize() {

        addView(new KinectView());
            
        return true;
    }
};

CNOID_IMPLEMENT_PLUGIN_ENTRY(KinectPlugin);
