//
// Created by MOSI000211 on 2025/6/17.
//

#include "MediaEncoder.h"

#include <android/log.h>
#include <media/NdkMediaFormat.h>

#define TAG "MediaEncoder"

const int FRAME_RATE = 30;
const float BPP = 0.25f;
const char *MIME_TYPE = "video/avc";

#define ANDROID_LOG(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

static int calcBitRate(int width, int height)
{
    int bitrate = (int) (BPP * FRAME_RATE * width * height);
    return bitrate;
}

MediaEncoder::MediaEncoder(int width, int height) : mWidth(width), mHeight(height)
{
    mIsEncoding = false;
    mFileFD = nullptr;
    mMediaMuxer = nullptr;
    mMediaCodec = nullptr;
    mTrackIndex = AMEDIA_ERROR_BASE;

    mEncoderThread = std::thread(std::bind(&MediaEncoder::encoderRun, this));
    pthread_setname_np(mEncoderThread.native_handle(), "EncoderThread");
}

void MediaEncoder::prepare(const std::string& filePath)
{
    AMediaFormat* format = AMediaFormat_new();
    AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, MIME_TYPE);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_FRAME_RATE, FRAME_RATE);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, mWidth);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, mHeight);
    //COLOR_FormatSurface: 0x7F000789
    //COLOR_FormatYUV420SemiPlanar(NV12): 0x15
    //COLOR_FormatYUV420PackedSemiPlanar(NV21): 0x27
    //COLOR_FormatYUV420Flexible: 0x7F420888
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, 0x15);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_BIT_RATE, calcBitRate(mWidth, mHeight));
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, 1);

    mMediaCodec = AMediaCodec_createEncoderByType(MIME_TYPE);
    AMediaCodec_configure(mMediaCodec, format, nullptr, nullptr, AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
    initialMediaMuxer(filePath);
    AMediaFormat_delete(format);
}

void MediaEncoder::start()
{
    media_status_t status;
    if (mMediaMuxer)
    {
        // 不能在这里start muxer, 否则在stop时会失败
        //status = AMediaMuxer_start(mMediaMuxer);
        //ANDROID_LOG("AMediaMuxer_start status: %d", status);
    }

    if (mMediaCodec)
    {
        status = AMediaCodec_start(mMediaCodec);
        ANDROID_LOG("AMediaCodec_start status: %d", status);
    }
}

void MediaEncoder::stop()
{
    mIsEncoding.store(false);
    mEncoderThread.join();
    media_status_t status;
    if (mMediaCodec)
    {
        status = AMediaCodec_stop(mMediaCodec);
        ANDROID_LOG("AMediaCodec_stop status: %d", status);
    }
    if (mMediaMuxer)
    {
        status = AMediaMuxer_stop(mMediaMuxer);
        ANDROID_LOG("AMediaMuxer_stop status: %d", status);
    }

    close();
}

void MediaEncoder::close()
{
    media_status_t status;
    if (mMediaMuxer)
    {
        status = AMediaMuxer_delete(mMediaMuxer);
        ANDROID_LOG("AMediaMuxer_delete status: %d", status);
    }
    if (mMediaCodec)
    {
        status = AMediaCodec_delete(mMediaCodec);
        ANDROID_LOG("AMediaCodec_delete status: %d", status);
    }
    if (mFileFD)
    {
        fclose(mFileFD);
        mFileFD = nullptr;
    }
    mMediaMuxer = nullptr;
    mMediaCodec = nullptr;
}

void MediaEncoder::push(uint8_t *data, int32_t size)
{
    mEncoderDataQueue.push(VideoInfo(data, size));
}

void MediaEncoder::encoderRun()
{
    mIsEncoding = true;
    while (mIsEncoding.load())
    {
        VideoInfo videoInfo;
        if (!mEncoderDataQueue.try_pop(videoInfo))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        ssize_t bufIndex = AMediaCodec_dequeueInputBuffer(mMediaCodec, 30*1000*1000);
        if (bufIndex >= 0) {
            size_t bufSize = 0;
            uint8_t* buf = AMediaCodec_getInputBuffer(mMediaCodec, bufIndex, &bufSize);
            assert(bufSize >= videoInfo.data_size);
            memcpy(buf, videoInfo.data, videoInfo.data_size);

            uint64_t time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            AMediaCodec_queueInputBuffer(mMediaCodec, bufIndex, 0, videoInfo.data_size, time, 0);
        }

        AMediaCodecBufferInfo info;
        ssize_t status = AMediaCodec_dequeueOutputBuffer(mMediaCodec, &info, 0);
        if (status >= 0) {
            if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM)
            {
                ANDROID_LOG("codec output EOS, flag=%x", info.flags);
            }

            size_t bufSize = 0;
            const uint8_t *bufOutput = AMediaCodec_getOutputBuffer(mMediaCodec, status, &bufSize);
            AMediaMuxer_writeSampleData(mMediaMuxer, mTrackIndex, bufOutput, &info);
            AMediaCodec_releaseOutputBuffer(mMediaCodec, status, false);
        }
        else if (status == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
            ANDROID_LOG("AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED.");
        }
        else if (status == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED)
        {
            AMediaFormat* format = AMediaCodec_getOutputFormat(mMediaCodec);
            mTrackIndex = AMediaMuxer_addTrack(mMediaMuxer, format);
            AMediaMuxer_start(mMediaMuxer);

            ANDROID_LOG("AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED.");
        }
        else if (status == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            ANDROID_LOG("AMEDIACODEC_INFO_TRY_AGAIN_LATER.");
            __android_log_print(ANDROID_LOG_INFO, TAG, "AMEDIACODEC_INFO_TRY_AGAIN_LATER");
        }
        else {
            ANDROID_LOG("unexpected info code: %zd", status);
        }
    }
}

bool MediaEncoder::initialMediaMuxer(const std::string &filePath)
{
    if (filePath.empty()) {
        ANDROID_LOG("initialMediaMuxer, file path is empty.");
        return false;
    }

    mFileFD = fopen(filePath.c_str(), "w+");
    if (nullptr == mFileFD) {
        ANDROID_LOG("initialMediaMuxer, open file failed.");
        return false;
    }

    int fd = fileno(mFileFD);
    fseek(mFileFD, 0L, SEEK_SET);
    mMediaMuxer = AMediaMuxer_new(fd, AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4);
    if (nullptr == mMediaMuxer) {
        ANDROID_LOG("initialMediaMuxer, create muxer failed.");
        return false;
    }
    return true;
}
