/******************************************************************************
 * China Entry Infotainment Project
 * Copyright (c) 2023 FAW-VW, P-VC & MOSI-Tech.
 * All Rights Reserved.
 *******************************************************************************/

/******************************************************************************
 * @file CameraCapture.kt
 * @ingroup  CameraPreview
 * @author fenghuaping
 * @brief This file is designed as xxxx service
 ******************************************************************************/
package com.fawvw.hmi.camerapreview.render

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.SurfaceTexture
import android.hardware.camera2.CameraAccessException
import android.hardware.camera2.CameraCaptureSession
import android.hardware.camera2.CameraCaptureSession.CaptureCallback
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraManager
import android.os.Handler
import android.util.Log
import android.util.Size
import android.view.Surface

class CameraCapture {
    companion object {
        private const val TAG = "CameraCapture"
    }
    private val mOpenCameraCallback: CameraDevice.StateCallback =
        object : CameraDevice.StateCallback() {
            override fun onOpened(camera: CameraDevice) {
                openCameraSession(camera)
            }

            override fun onDisconnected(camera: CameraDevice) {}
            override fun onError(camera: CameraDevice, error: Int) {}
        }
    private val mCreateSessionCallback: CameraCaptureSession.StateCallback =
        object : CameraCaptureSession.StateCallback() {
            override fun onConfigured(session: CameraCaptureSession) {
                mCameraCaptureSession = session
                requestPreview(session)
            }

            override fun onConfigureFailed(session: CameraCaptureSession) {}
        }
    private var mCameraDevice: CameraDevice? = null
    private var mCameraCaptureSession: CameraCaptureSession? = null
    private var mPreviewSurface: Surface? = null
    private var mCaptureCallback: CaptureCallback? = null
    private var mHandler: Handler? = null
    @SuppressLint("MissingPermission")
    fun openCamera(
        context: Context,
        preview: SurfaceTexture,
        facing: Int,
        width: Int,
        height: Int,
        captureCallback: CaptureCallback?,
        handler: Handler?
    ) {
        mPreviewSurface = Surface(preview)
        mCaptureCallback = captureCallback
        mHandler = handler
        val manager = context.getSystemService(Context.CAMERA_SERVICE) as CameraManager
        try {
            for (id in manager.cameraIdList) {
                val cc = manager.getCameraCharacteristics(id!!)
                if (cc.get(CameraCharacteristics.LENS_FACING) == facing) {

                    val streamMap = cc.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP) ?: return
                    val previewSizes = streamMap.getOutputSizes(SurfaceTexture::class.java)

                    val previewSize = getMostSuitableSize(previewSizes, width.toFloat(), height.toFloat()) ?: return
                    preview.setDefaultBufferSize(previewSize.width, previewSize.height)
                    manager.openCamera(id, mOpenCameraCallback, mHandler)
                    break
                }
            }
        } catch (e: Exception) {
            Log.e(TAG, "can not open camera", e)
        }
    }

    fun closeCamera() {
        if (mCameraCaptureSession != null) {
            mCameraCaptureSession!!.close()
            mCameraCaptureSession = null
        }
        if (mCameraDevice != null) {
            mCameraDevice!!.close()
            mCameraDevice = null
        }
    }

    private fun getMostSuitableSize(sizes: Array<Size>, width: Float, height: Float): Size? {
        val targetRatio = height / width
        var result: Size? = null
        for (size in sizes) {
            if (result == null || isMoreSuitable(result, size, targetRatio)) {
                result = size
            }
        }
        return result
    }

    private fun isMoreSuitable(current: Size?, target: Size, targetRatio: Float): Boolean {
        if (current == null) {
            return true
        }
        val dRatioTarget = Math.abs(targetRatio - getRatio(target))
        val dRatioCurrent = Math.abs(targetRatio - getRatio(current))
        return dRatioTarget < dRatioCurrent || dRatioTarget == dRatioCurrent && getArea(target) > getArea(
            current
        )
    }

    private fun getArea(size: Size): Int {
        return size.width * size.height
    }

    private fun getRatio(size: Size): Float {
        return size.width.toFloat() / size.height
    }

    private fun openCameraSession(camera: CameraDevice) {
        mCameraDevice = camera
        try {
            val outputs = listOf(mPreviewSurface)
            camera.createCaptureSession(outputs, mCreateSessionCallback, mHandler)
        } catch (e: CameraAccessException) {
            Log.e(TAG, "createCaptureSession failed", e)
        }
    }

    private fun requestPreview(session: CameraCaptureSession) {
        if (mCameraDevice == null) {
            return
        }
        try {
            val builder = mCameraDevice!!.createCaptureRequest(
                CameraDevice.TEMPLATE_PREVIEW
            )
            builder.addTarget(mPreviewSurface!!)
            session.setRepeatingRequest(builder.build(), mCaptureCallback, mHandler)
        } catch (e: CameraAccessException) {
            Log.e(TAG, "requestPreview failed", e)
        }
    }
}