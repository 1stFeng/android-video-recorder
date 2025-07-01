//
// Created by MOSI000211 on 2025/6/17.
//

#ifndef CAMERAPREVIEW_MEDIAENCODER_H
#define CAMERAPREVIEW_MEDIAENCODER_H

#include "ThreadSafeQueue.h"

#include <media/NdkMediaMuxer.h>
#include <media/NdkMediaCodec.h>
#include <condition_variable>
#include <mutex>

class VideoInfo {
public:
    VideoInfo() = default;
    VideoInfo(uint8_t *data_, int32_t size_) {
        data = data_;
        data_size = size_;
    }

    uint8_t *data = nullptr;
    int32_t data_size = 0;
};

class MediaEncoder {
public:
    MediaEncoder(int width, int height);

    void prepare(const std::string& filePath);
    void start();
    void stop();
    void close();
    void push(uint8_t *data, int32_t size);

    void encoderRun();

private:
    bool initialMediaMuxer(const std::string& filePath);

private:
    int mWidth;
    int mHeight;
    ssize_t mTrackIndex;
    FILE* mFileFD;
    std::atomic_bool mIsEncoding;
    AMediaMuxer* mMediaMuxer;
    AMediaCodec* mMediaCodec;
    std::thread mEncoderThread;
    ThreadSafeQueue<VideoInfo> mEncoderDataQueue;
};


#endif //CAMERAPREVIEW_MEDIAENCODER_H
