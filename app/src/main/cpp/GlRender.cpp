//
// Created by MOSI000211 on 2025/6/6.
//

#include "GlRender.h"
#include "GlShaderString.h"
#include <android/log.h>

#define TAG "GlRender"

float vertices_texture[] = {
        //---- 顶点坐标 ----                            - 纹理坐标 -
        -1.0f,  -1.0f, 0.0f,		0.0f, 1.0f,   // 左下
        1.0f, -1.0f, 0.0f,		1.0f, 1.0f,   // 右下
        -1.0f,  1.0f, 0.0f,		0.0f, 0.0f,   // 左上
        1.0f,  1.0f, 0.0f,		1.0f, 0.0f    // 右上
};

unsigned int glCheckError_(const char* file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (errorCode)
        {
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
            default:                               error = "UNKNOWN"; break;
        }
        __android_log_print(ANDROID_LOG_INFO, TAG,"glCheckError: %d - %s|%s(%d).", errorCode, error.c_str(), file, line);
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

unsigned int genTexture()
{
    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    return texture;
}

GlRender::GlRender()
{
    mVertexArrayObject = 0;
    eglPresentationTimeANDROID = nullptr;
}

void GlRender::initOpenGL()
{
    const char* versionStr = (const char*)glGetString(GL_VERSION);
    __android_log_print(ANDROID_LOG_INFO, TAG,"opengl es version: %s", versionStr);

    glClearColor(0.f, 0.f, 0.f, 1.f);
    mTextureY = genTexture();
    mTextureUV = genTexture();

    if (eglGetCurrentContext() == EGL_NO_CONTEXT)
    {
        __android_log_print(ANDROID_LOG_ERROR, TAG,"eglGetCurrentContext error.");
    }
    if (eglGetCurrentContext() != mEGLContext)
    {
        __android_log_print(ANDROID_LOG_ERROR, TAG,"eglGetCurrentContext not equal mEGLContext.");
    }

    mShaderPtr = std::make_shared<GlShader>(NV12VertexShader, NV12FragmentShader);
    mShaderPtr->bind();
    glGenVertexArrays(1, &mVertexArrayObject);
    glBindVertexArray(mVertexArrayObject);

    /* 创建一个缓冲对象 */
    unsigned int vbo;
    glGenBuffers(1, &vbo);

    /* 将缓冲对象绑定到顶点缓冲对象上 */
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    /* 将vertices顶点数据复制到缓冲对象内存中 */
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 20, vertices_texture, GL_STATIC_DRAW);

    /* 启用顶点属性数组 */
    GLuint location = mShaderPtr->getAttribLocation("aPosition");
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 0));

    location = mShaderPtr->getAttribLocation("aTextureCoord");
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 3));
}

EGLSurface GlRender::initEGL(ANativeWindow* nativeWindow) {
    int windowWidth;
    int windowHeight;

    EGLint configSpec[] = {
            EGL_ALPHA_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_NONE
    };

    mEGLDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mEGLDisplay == EGL_NO_DISPLAY)
    {
        __android_log_print(ANDROID_LOG_ERROR, TAG,"can't get eglGetDisplay.");
        return nullptr;
    }
    EGLint numConfigs;
    EGLint eglMajVers = 0, eglMinVers = 0;
    eglInitialize(mEGLDisplay, &eglMajVers, &eglMinVers);
    eglChooseConfig(mEGLDisplay, configSpec, &mEGLConfig, 1, &numConfigs);

    const EGLint ctxAttr[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
    };
    mEGLContext = eglCreateContext(mEGLDisplay, mEGLConfig, EGL_NO_CONTEXT, ctxAttr);
    if (mEGLContext == EGL_NO_CONTEXT)
    {
        __android_log_print(ANDROID_LOG_ERROR, TAG,"can't eglCreateContext.");
        return nullptr;
    }

    mEGLSurface = createEGLSurface(nativeWindow);
    eglQuerySurface(mEGLDisplay, mEGLSurface, EGL_WIDTH, &windowWidth);
    eglQuerySurface(mEGLDisplay, mEGLSurface, EGL_HEIGHT, &windowHeight);
    __android_log_print(ANDROID_LOG_ERROR, TAG,"createEGLSurface width=%d, height=%d.", windowWidth, windowHeight);

    // 加载eglPresentationTimeANDROID函数
    eglPresentationTimeANDROID = (PFNEGLPRESENTATIONTIMEANDROIDPROC) eglGetProcAddress("eglPresentationTimeANDROID");
    if (!eglPresentationTimeANDROID) {
        __android_log_print(ANDROID_LOG_ERROR, TAG,"eglPresentationTimeANDROID extension not available");
    }
    makeCurrent(mEGLSurface);
    glViewport(0, 0, windowWidth, windowHeight);
    return mEGLSurface;
}

void GlRender::makeCurrent(EGLSurface eglSurface)
{
    eglMakeCurrent(mEGLDisplay, eglSurface, eglSurface, mEGLContext);
}

void GlRender::setPresentTime(EGLSurface eglSurface, long nsecs)
{
    if (eglPresentationTimeANDROID)
    {
        eglPresentationTimeANDROID(mEGLDisplay, eglSurface, nsecs);
    }
}

EGLSurface GlRender::createEGLSurface(ANativeWindow* nativeWindow)
{
    if (nativeWindow == nullptr) return nullptr;
    return eglCreateWindowSurface(mEGLDisplay, mEGLConfig, nativeWindow, nullptr);
}

void GlRender::destroyEGLSurface(EGLSurface eglSurface)
{
    if (eglSurface == nullptr) return;
    eglDestroySurface(mEGLDisplay, eglSurface);
}

void GlRender::render(EGLSurface eglSurface)
{
    makeCurrent(eglSurface);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTextureY);
    mShaderPtr->setInt("tex2D_y", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mTextureUV);
    mShaderPtr->setInt("tex2D_uv", 1);

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);

    eglSwapBuffers(mEGLDisplay, eglSurface);
}

bool GlRender::uploadNV21Buffer(const std::vector<unsigned char> &buffer, int width, int height) const
{
    if (buffer.size() != width * height * 3 / 2)
    {
        __android_log_print(ANDROID_LOG_ERROR, TAG,"uploadYUVBuffer error: not nv21 buffer.");
        return false;
    }
    const uint8_t* yData = buffer.data();
    const uint8_t* uvData = buffer.data() + width * height;

    glBindTexture(GL_TEXTURE_2D, mTextureY);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, yData);
    glCheckError();

    glBindTexture(GL_TEXTURE_2D, mTextureUV);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, width / 2, height / 2, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, uvData);
    glCheckError();
    return true;
}

void GlRender::setCropSize(float left, float top, float right, float bottom)
{
    vertices_texture[3] = left;
    vertices_texture[4] = top;
    vertices_texture[8] = right;
    vertices_texture[9] = top;
    vertices_texture[13] = left;
    vertices_texture[14] = bottom;
    vertices_texture[18] = right;
    vertices_texture[19] = bottom;
}
