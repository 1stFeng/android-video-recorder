//
// Created by MOSI000211 on 2025/6/6.
//

#ifndef CAMERAPREVIEW_GLRENDER_H
#define CAMERAPREVIEW_GLRENDER_H

#include <jni.h>
#include <vector>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <android/native_window.h>

#include "GlShader.h"

class GlRender {
public:
    GlRender();
    void initOpenGL();
    void makeCurrent(EGLSurface eglSurface);
    EGLSurface initEGL(ANativeWindow* nativeWindow);
    EGLSurface createEGLSurface(ANativeWindow* nativeWindow);
    void destroyEGLSurface(EGLSurface eglSurface);
    void setCropSize(float left, float top, float right, float bottom);
    void setPresentTime(EGLSurface eglSurface, long nsecs);
    void render(EGLSurface eglSurface);
    bool uploadNV21Buffer(const std::vector<unsigned char>& buffer, int width, int height) const;

private:
    unsigned int mTextureY;
    unsigned int mTextureUV;
    unsigned int mVertexArrayObject;
    std::shared_ptr<GlShader> mShaderPtr;

    EGLContext mEGLContext{};
    EGLDisplay mEGLDisplay{};
    EGLConfig mEGLConfig{};
    EGLSurface mEGLSurface{};
    PFNEGLPRESENTATIONTIMEANDROIDPROC eglPresentationTimeANDROID;
};


#endif //CAMERAPREVIEW_GLRENDER_H
