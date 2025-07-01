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
import android.hardware.camera2.CameraCaptureSession
import android.hardware.camera2.CameraCaptureSession.CaptureCallback
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.TotalCaptureResult
import android.media.MediaRecorder
import android.opengl.EGLSurface
import android.os.Bundle
import android.os.Environment
import android.os.Handler
import android.os.HandlerThread
import android.view.SurfaceView
import android.view.TextureView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.core.content.PermissionChecker
import com.fawvw.hmi.camerapreview.databinding.ActivityMainBinding
import com.fawvw.hmi.camerapreview.render.CameraCapture
import com.fawvw.hmi.camerapreview.render.GLRender
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.lang.Exception


class MainCameraActivity : AppCompatActivity() {
    companion object {
        private const val TAG = "MainCameraActivity"
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

    private val mCameraCapture = CameraCapture()
    private val mRenderThread = HandlerThread("render")
    private lateinit var mRenderHandler: Handler

    private lateinit var mPreviewView: TextureView
    private var mPreviewTexture: SurfaceTexture? = null
    private var mCameraTexture: SurfaceTexture? = null
    private var mIsRunning = false

    private var mRecordSurface: EGLSurface? = null
    private var mMediaRecorder: MediaRecorder? = null
    private lateinit var mLastVideo: File

    private val mCaptureCallback: CaptureCallback = object : CaptureCallback() {
        override fun onCaptureCompleted(
            session: CameraCaptureSession,
            request: CaptureRequest,
            result: TotalCaptureResult
        ) {
            val mTransformMatrix = FloatArray(16) { 0.0f }
            super.onCaptureCompleted(session, request, result)
            mCameraTexture!!.updateTexImage()
            mCameraTexture!!.getTransformMatrix(mTransformMatrix)
            mGLRender.render(mTransformMatrix, mGLRender.defaultEGLSurface)
            if (mRecordSurface != null) {
                mGLRender.render(mTransformMatrix, mRecordSurface)
                mGLRender.setPresentationTime(mRecordSurface, mPreviewTexture!!.timestamp)
            }
        }
    }

    private val mPreViewCallback = object : TextureView.SurfaceTextureListener {
        override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
            mPreviewTexture = surface
            mRenderHandler.post {
                initRender(surface, width, height)
                checkPermissionsAndOpenCamera()
            }
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
    private val mGLRender = GLRender()
    private lateinit var binding: ActivityMainBinding
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        val view = binding.root
        setContentView(view)
        if (ContextCompat.checkSelfPermission(this, PERMISSIONS[0]) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(PERMISSIONS, 0)
        }

        mRenderThread.start()
        mRenderHandler = Handler(mRenderThread.looper)

        mPreviewView = TextureView(this)
        mPreviewView.surfaceTextureListener = mPreViewCallback
        binding.clPreContainer.addView(mPreviewView)
        handleEvent()
    }

    private fun handleEvent() {
        binding.btnStartPreview.setOnClickListener {

        }

        binding.btnStartRecord.setOnClickListener {
            if (mMediaRecorder == null) {
                if (!grantedRecordPermission()) {
                    return@setOnClickListener
                }
                mRecordSurface = startRecord(genFileName())
                Toast.makeText(this, "start record", Toast.LENGTH_SHORT).show()
            } else {
                stopRecord()
                val tips = "save video to " + mLastVideo.absolutePath
                Toast.makeText(this, tips, Toast.LENGTH_SHORT).show()
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

    private fun initRender(surface: SurfaceTexture, width: Int, height: Int) {
        mGLRender.init(surface, width, height)
        mCameraTexture = SurfaceTexture(mGLRender.texture)
    }

    private fun checkPermissionsAndOpenCamera() {
        val check = PermissionChecker.checkSelfPermission(this, Manifest.permission.CAMERA)
        if (check == PermissionChecker.PERMISSION_GRANTED) {
            openCamera()
        } else {
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CODE)
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (permissions.size > 1 && permissions[0] == Manifest.permission.CAMERA && grantResults[0] == PermissionChecker.PERMISSION_GRANTED) {
            openCamera()
        }
    }

    override fun onResume() {
        super.onResume()
        if (mCameraTexture != null && !mIsRunning) {
            openCamera()
        }
    }

    private fun openCamera() {
        mIsRunning = true
        mCameraCapture.openCamera(
            this,
            mCameraTexture!!,
            CameraCharacteristics.LENS_FACING_BACK,
            mPreviewView.width,
            mPreviewView.height,
            mCaptureCallback,
            mRenderHandler
        )
    }

    override fun onPause() {
        super.onPause()
        if (mIsRunning) {
            closeCamera()
        }
        if (mMediaRecorder != null) {
            stopRecord()
        }
    }

    private fun closeCamera() {
        mIsRunning = false
        mCameraCapture.closeCamera()
    }

    @SuppressLint("SetTextI18n")
    private fun startRecord(filename: String): EGLSurface {
        mLastVideo = File(externalCacheDir, filename)
        mMediaRecorder = MediaRecorder()
        mMediaRecorder!!.setAudioSource(MediaRecorder.AudioSource.CAMCORDER)
        mMediaRecorder!!.setVideoSource(MediaRecorder.VideoSource.SURFACE)
        mMediaRecorder!!.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4)
        mMediaRecorder!!.setOutputFile(mLastVideo.path)
        mMediaRecorder!!.setVideoEncoder(MediaRecorder.VideoEncoder.H264)
        mMediaRecorder!!.setVideoEncodingBitRate(VIDEO_BIT_RATE)
        mMediaRecorder!!.setVideoSize(mPreviewView.width, mPreviewView.height)
        mMediaRecorder!!.setVideoFrameRate(VIDEO_FRAME_RATE)
        mMediaRecorder!!.setAudioEncoder(MediaRecorder.AudioEncoder.AAC)
        mMediaRecorder!!.setAudioEncodingBitRate(AUDIO_BIT_RATE)
        mMediaRecorder!!.setOrientationHint(0)
        try {
            mMediaRecorder!!.prepare()
        } catch (e: IOException) {
            e.printStackTrace()
        }
        mMediaRecorder!!.start()
        binding.btnStartRecord.text = "StopRecord"
        return mGLRender.createEGLSurface(mMediaRecorder!!.surface)
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

    private fun checkMediaRecorderPermission(): Boolean {
        var check = PermissionChecker.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)
        if (check == PermissionChecker.PERMISSION_DENIED) {
            requestPermissions(
                arrayOf(Manifest.permission.RECORD_AUDIO),
                REQUEST_CODE
            )
            return false
        }
        check = PermissionChecker.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
        if (check == PermissionChecker.PERMISSION_DENIED) {
            requestPermissions(arrayOf(Manifest.permission.WRITE_EXTERNAL_STORAGE), REQUEST_CODE)
            return false
        }
        return true
    }

    private fun stopRecord() {
        mMediaRecorder!!.stop()
        mMediaRecorder!!.release()
        mMediaRecorder = null
        mGLRender.destroyEGLSurface(mRecordSurface)
        mRecordSurface = null
        binding.btnStartRecord.text = "StartRecord"
    }

    private fun genFileName(): String {
        return "video_" + System.currentTimeMillis() + ".mp4"
    }
}