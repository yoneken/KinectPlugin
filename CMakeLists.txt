# @author Kenta Yonekura (yoneken)

if(MSVC)
	option(BUILD_KINECT_PLUGIN "Building a Kinect plugin" OFF)
	set(sources KinectPlugin.cpp KinectView.cpp KinectImpl.cpp FaceTracker.cpp)
	set(headers KinectView.h KinectImpl.h FaceTracker.h)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR} $ENV{KINECTSDK10_DIR}/inc $ENV{FTSDK_DIR}inc)

	if(BUILD_KINECT_PLUGIN)
		set(target CnoidKinectPlugin)
		QT4_WRAP_CPP(sources KinectView.h)
		link_directories($ENV{FTSDK_DIR}Lib/x86 $ENV{KINECTSDK10_DIR}lib/x86)
		add_library(${target} SHARED ${sources} ${headers})
		#add_cnoid_plugin(${target} SHARED ${sources})
		target_link_libraries(${target} CnoidBase )
		apply_common_setting_for_plugin(${target} "${headers}")
	endif()

endif()