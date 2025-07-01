//
// Created by MOSI000211 on 2025/6/9.
//

#include "PreviewWrapper.h"
#include <fstream>
#include <android/log.h>

#define TAG "PreviewWrapper"

const unsigned int RENDER_INITIAL     =  1;
const unsigned int RENDER_INITIALED   =  1 << 1;
const unsigned int PREVIEW_STARTED    =  1 << 2;
const unsigned int RECORD_STARTED     =  1 << 3;

PreviewWrapper::PreviewWrapper()
{
    mRunningState.store(0);
    mRecordSurface = nullptr;
    mPreviewSurface = nullptr;
    mNativeWindow = nullptr;
    mPlayerThread = std::thread(std::bind(&PreviewWrapper::run, this));
    pthread_setname_np(mPlayerThread.native_handle(), "PreviewThread");
}

void PreviewWrapper::initRender(ANativeWindow* nativeWindow)
{
    // opengl 初始化相关操作必须放在子线程
    mNativeWindow = nativeWindow;
    uint32_t state = mRunningState.load();
    mRunningState.store(state | RENDER_INITIAL);
}

void PreviewWrapper::stopRecord()
{
    mGlRender.destroyEGLSurface(mRecordSurface);
    mRecordSurface = nullptr;
    uint32_t state = mRunningState.load();
    mRunningState.store(state & ~RECORD_STARTED);
    //mMediaEncoder->stop();
}

void PreviewWrapper::prepareRecord(const std::string& path)
{
    mMediaEncoder.reset(new MediaEncoder(596, 336));
    mMediaEncoder->prepare(path);
}

void PreviewWrapper::startRecord(ANativeWindow* nativeWindow)
{
    //mMediaEncoder->start();
    mRecordSurface = mGlRender.createEGLSurface(nativeWindow);
    uint32_t state = mRunningState.load();
    mRunningState.store(state | RECORD_STARTED);
}

void PreviewWrapper::stopPreview()
{
    uint32_t state = mRunningState.load();
    mRunningState.store(state & ~PREVIEW_STARTED);
}

void PreviewWrapper::startReadDataFromFile()
{
    uint32_t state = mRunningState.load();
    mRunningState.store(state | PREVIEW_STARTED);
}

void PreviewWrapper::run() {
    int width = 596, height = 336;
    const int frameSize = width * height * 3 / 2; // 每帧大小
    std::vector<unsigned char> frameData(frameSize);
    while (true) {
        uint32_t state = mRunningState.load();
        if ((state & RENDER_INITIAL) != RENDER_INITIAL) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        if ((state & RENDER_INITIALED) != RENDER_INITIALED) {
            //(40, 100, 386, 170)
            mGlRender.setCropSize(40 / 596.0f, 1.0f - 100 / 336.0f, 426 / 596.0f, 1.0f - 270 / 336.0f);
            mPreviewSurface = mGlRender.initEGL(mNativeWindow);
            mGlRender.initOpenGL();

            mRunningState.store(state | RENDER_INITIALED);
        }

        if ((state & PREVIEW_STARTED) != PREVIEW_STARTED) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }
        std::string fileName = "/sdcard/Android/data/com.fawvw.hmi.camerapreview/cache/dvr_nv12.yuv";
        std::ifstream file(fileName, std::ios::binary);
        if (!file) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "yuv file open failed.");
            return;
        }

        // 获取文件总大小
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        // 计算总帧数
        size_t totalFrames = fileSize / frameSize;
        for (int i = 0; i < totalFrames && (mRunningState.load() & PREVIEW_STARTED); ++i)
        {
            file.read(reinterpret_cast<char *>(frameData.data()), frameSize);
            if (mGlRender.uploadNV21Buffer(frameData, width, height))
            {
                mGlRender.render(mPreviewSurface);
                if (mRunningState.load() & RECORD_STARTED)
                {
                    //mMediaEncoder->push(frameData.data(), frameSize);

                    mGlRender.render(mRecordSurface);
                    //mGlRender.setPresentTime(mRecordSurface,std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
        file.close();
    }
}
