//
// Created by MOSI000211 on 2025/6/9.
//

#ifndef CAMERAPREVIEW_PREVIEWWRAPPER_H
#define CAMERAPREVIEW_PREVIEWWRAPPER_H

#include "GlRender.h"
#include "MediaEncoder.h"
#include <thread>

class PreviewWrapper {
public:
    PreviewWrapper();
    void initRender(ANativeWindow* nativeWindow);

    void run();
    void stopRecord();
    void prepareRecord(const std::string& path);
    void startRecord(ANativeWindow* nativeWindow);
    void stopPreview();
    void startReadDataFromFile();

private:
    GlRender mGlRender;
    std::thread mPlayerThread;
    EGLSurface mPreviewSurface;
    EGLSurface mRecordSurface;
    std::atomic_uint mRunningState;
    ANativeWindow* mNativeWindow;
    std::unique_ptr<MediaEncoder> mMediaEncoder;
};


#endif //CAMERAPREVIEW_PREVIEWWRAPPER_H
