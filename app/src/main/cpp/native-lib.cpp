#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "PreviewWrapper.h"

std::unique_ptr<PreviewWrapper> gPreViewWrapper(new PreviewWrapper);

extern "C"
JNIEXPORT void JNICALL
Java_com_fawvw_hmi_camerapreview_jni_NativeHelper_nativeSetPreviewSurface(JNIEnv *env, jobject thiz,
                                                                          jobject surface) {

    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    gPreViewWrapper->initRender(nativeWindow);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_fawvw_hmi_camerapreview_jni_NativeHelper_nativeStartRecord(JNIEnv *env, jobject thiz,
                                                                    jobject surface) {
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    gPreViewWrapper->startRecord(nativeWindow);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_fawvw_hmi_camerapreview_jni_NativeHelper_nativeStopRecord(JNIEnv *env, jobject thiz) {
    gPreViewWrapper->stopRecord();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_fawvw_hmi_camerapreview_jni_NativeHelper_nativeStartPreview(JNIEnv *env, jobject thiz) {
    gPreViewWrapper->startReadDataFromFile();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_fawvw_hmi_camerapreview_jni_NativeHelper_nativeStopPreview(JNIEnv *env, jobject thiz) {
    gPreViewWrapper->stopPreview();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_fawvw_hmi_camerapreview_jni_NativeHelper_nativeStartEncoder(JNIEnv *env, jobject thiz) {
    gPreViewWrapper->startRecord(nullptr);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_fawvw_hmi_camerapreview_jni_NativeHelper_nativePrepareEncoder(JNIEnv *env, jobject thiz,
                                                                       jstring path) {
    const char* strPath = env->GetStringUTFChars(path, nullptr);
    gPreViewWrapper->prepareRecord(strPath);
    env->ReleaseStringUTFChars(path, strPath);
}