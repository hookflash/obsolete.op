
#include "engine_configurations.h"

#ifndef WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_IPHONE_H_
#define WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_IPHONE_H_

#include "vie_autotest_window_manager_interface.h"
#include "iphone_render_view.h"

class ViEAutoTestWindowManager: public ViEAutoTestWindowManagerInterface
{
public:
    ViEAutoTestWindowManager(IPhoneRenderView* renderView1, IPhoneRenderView* renderView2);
    ~ViEAutoTestWindowManager();
    virtual void* GetWindow1();
    virtual void* GetWindow2();
    virtual int CreateWindows(AutoTestRect window1Size,
                              AutoTestRect window2Size, void* window1Title,
                              void* window2Title);
    virtual int TerminateWindows();
    virtual bool SetTopmostWindow();
    
private:
    IPhoneRenderView* _iPhoneRenderView1;
    IPhoneRenderView* _iPhoneRenderView2;
};

#endif  // WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_IPHONE_H_
