/******************************************************************************
 * China Entry Infotainment Project
 * Copyright (c) 2023 FAW-VW, P-VC & MOSI-Tech.
 * All Rights Reserved.
 *******************************************************************************/

/******************************************************************************
 * @file MainCameraActivity.kt
 * @ingroup  CameraPreview
 * @author fenghuaping
 * @brief This file is designed as xxxx service
 ******************************************************************************/
package com.fawvw.hmi.camerapreview

import android.Manifest
import android.annotation.SuppressLint
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.graphics.SurfaceTexture
import android.media.MediaMuxer
import android.media.MediaRecorder
import android.os.Bundle
import android.util.Log
import android.view.Surface
import android.view.SurfaceHolder
import android.view.TextureView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.core.content.PermissionChecker
import com.fawvw.hmi.camerapreview.databinding.ActivityMainBinding
import com.fawvw.hmi.camerapreview.jni.NativeHelper
import java.io.File
import java.io.FileOutputStream
import java.io.IOException


class MainNativeActivity : AppCompatActivity() {
    companion object {
        private const val TAG = "MainNativeActivity"
        private const val REQUEST_CODE = 0x1
        private const val VIDEO_BIT_RATE = 1024 * 1024 * 1024
        private const val VIDEO_FRAME_RATE = 30
        private const val AUDIO_BIT_RATE = 44800
    }

    private val PERMISSIONS = arrayOf(
        Manifest.permission.RECORD_AUDIO,
        Manifest.permission.CAMERA,
        Manifest.permission.READ_EXTERNAL_STORAGE,
        Manifest.permission.WRITE_EXTERNAL_STORAGE
    )

    private lateinit var mPreviewView: TextureView
    private var mMediaRecorder: MediaRecorder? = null
    private lateinit var mLastVideo: File

    private val mPreviewCallback = object : SurfaceHolder.Callback {
        override fun surfaceCreated(holder: SurfaceHolder) {
            NativeHelper.nativeSetPreviewSurface(holder.surface)
            NativeHelper.nativeStartPreview()
        }

        override fun surfaceChanged(holder: SurfaceHolder, p1: Int, p2: Int, p3: Int) {
            // do nothing
        }

        override fun surfaceDestroyed(holder: SurfaceHolder) {
            // do nothing
        }
    }

    private val mTextureViewCallback = object : TextureView.SurfaceTextureListener {
        override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
            NativeHelper.nativeSetPreviewSurface(Surface(surface))
            NativeHelper.nativeStartPreview()
        }

        override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, p1: Int, p2: Int) {
            // do nothing
        }

        override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
            return false
        }

        override fun onSurfaceTextureUpdated(surface: SurfaceTexture) {
            // do nothing
        }
    }

    private lateinit var binding: ActivityMainBinding
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        val view = binding.root
        setContentView(view)
        if (ContextCompat.checkSelfPermission(this, PERMISSIONS[0]) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(PERMISSIONS, 0)
        }

        mPreviewView = TextureView(this)
        mPreviewView.surfaceTextureListener = mTextureViewCallback
        binding.clPreContainer.addView(mPreviewView)
        handleEvent()
    }

    private fun handleEvent() {
        binding.btnStartPreview.setOnClickListener {

        }

        binding.btnStartRecord.setOnClickListener {
            if (binding.btnStartRecord.text == "StopRecord") {
                stopRecord()
                val tips = "save video to " + mLastVideo.absolutePath
                Toast.makeText(this, tips, Toast.LENGTH_SHORT).show()
            } else {
                if (!grantedRecordPermission()) {
                    return@setOnClickListener
                }
                mLastVideo = File(externalCacheDir, genFileName())
                //1.使用MediaRecorder录像,在开始和结束会卡顿一下，影响体验
                val mRecordSurface = startRecord()
                NativeHelper.nativeStartRecord(mRecordSurface)

                //1.使用MediaCodec录像，需要手动裁剪需要的视频画面
                //NativeHelper.nativePrepareEncoder(mLastVideo.path)
                //NativeHelper.nativeStartEncoder()
                binding.btnStartRecord.text = "StopRecord"
                Toast.makeText(this, "start record", Toast.LENGTH_SHORT).show()
            }
        }

        binding.btnStartPhoto.setOnClickListener {
            val file = File(externalCacheDir, "image_${System.currentTimeMillis()}.bmp")
            val photo = mPreviewView.bitmap ?: return@setOnClickListener
            try {
                val outputStream = FileOutputStream(file)
                photo.compress(Bitmap.CompressFormat.JPEG, 80, outputStream)
                outputStream.flush()
                outputStream.close()
            } catch (exception: Exception) {
                exception.printStackTrace()
            }
        }
    }

    override fun onPause() {
        super.onPause()
        if (mMediaRecorder != null) {
            stopRecord()
        }
    }

    @SuppressLint("SetTextI18n")
    private fun startRecord(): Surface {
        mMediaRecorder = MediaRecorder()
        mMediaRecorder!!.setVideoSource(MediaRecorder.VideoSource.SURFACE)
        mMediaRecorder!!.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4)
        mMediaRecorder!!.setOutputFile(mLastVideo.path)
        mMediaRecorder!!.setVideoEncoder(MediaRecorder.VideoEncoder.H264)
        mMediaRecorder!!.setVideoEncodingBitRate(VIDEO_BIT_RATE)
        mMediaRecorder!!.setVideoSize(mPreviewView.width, mPreviewView.height)
        mMediaRecorder!!.setVideoFrameRate(VIDEO_FRAME_RATE)
        mMediaRecorder!!.setOrientationHint(0)
        mMediaRecorder!!.setOnErrorListener { _, what, extra ->
            Log.e(TAG, "media recorder error: $what, $extra")
        }
        mMediaRecorder!!.setOnInfoListener { _, what, extra ->
            Log.w(TAG, "media recorder info: $what, $extra")
        }
        try {
            mMediaRecorder!!.prepare()
        } catch (e: IOException) {
            e.printStackTrace()
        }
        mMediaRecorder!!.start()
        return mMediaRecorder!!.surface
    }

    private fun grantedRecordPermission(): Boolean {
        val permissions: MutableList<String> = ArrayList()
        val audioPermission = PermissionChecker.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)
        if (audioPermission == PermissionChecker.PERMISSION_DENIED) {
            permissions.add(Manifest.permission.RECORD_AUDIO)
        }
        val audioWrite = PermissionChecker.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
        if (audioWrite == PermissionChecker.PERMISSION_DENIED) {
            permissions.add(Manifest.permission.WRITE_EXTERNAL_STORAGE)
        }
        if (permissions.isEmpty()) {
            return true
        }
        requestPermissions(permissions.toTypedArray(), REQUEST_CODE)
        return false
    }

    private fun stopRecord() {
        mMediaRecorder?.stop()
        mMediaRecorder?.release()
        mMediaRecorder = null
        NativeHelper.nativeStopRecord()
        binding.btnStartRecord.text = "StartRecord"
    }

    private fun genFileName(): String {
        return "video_" + System.currentTimeMillis() + ".mp4"
    }
}