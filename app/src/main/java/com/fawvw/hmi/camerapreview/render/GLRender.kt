/******************************************************************************
 * China Entry Infotainment Project
 * Copyright (c) 2023 FAW-VW, P-VC & MOSI-Tech.
 * All Rights Reserved.
 *******************************************************************************/

/******************************************************************************
 * @file GlRender.kt
 * @ingroup  CameraPreview
 * @author fenghuaping
 * @brief This file is designed as xxxx service
 ******************************************************************************/
package com.fawvw.hmi.camerapreview.render

import android.graphics.SurfaceTexture
import android.opengl.EGL14
import android.opengl.EGLConfig
import android.opengl.EGLContext
import android.opengl.EGLDisplay
import android.opengl.EGLExt
import android.opengl.EGLSurface
import android.opengl.GLES11Ext
import android.opengl.GLES20
import android.view.Surface
import com.fawvw.hmi.camerapreview.utils.EglUtil

class GLRender {
    companion object {
        private const val TAG = "CameraRenderer"
        private const val VERTEX_SHADER = "attribute vec4 aPosition;\n" +
                "attribute vec4 aTextureCoord;\n" +
                "varying vec2 vTextureCoord;\n" +
                "uniform mat4 matTransform;\n" +
                "void main() {\n" +
                "  gl_Position = aPosition;\n" +
                "  vTextureCoord = (matTransform * vec4(aTextureCoord.xy, 0, 1)).xy;\n" +
                "}"

        private const val FRAGMENT_SHADER = "#extension GL_OES_EGL_image_external : require\n" +
                "precision mediump float;\n" +
                "varying vec2 vTextureCoord;\n" +
                "uniform mat4 uColorMatrix;\n" +
                "uniform samplerExternalOES sTexture;\n" +
                "void main() {\n" +
                "  gl_FragColor = uColorMatrix * texture2D(sTexture, vTextureCoord).rgba;\n" +
                "}"

        //反相
        private val COLOR_MATRIX = floatArrayOf(
            -1.0f, 0.0f, 0.0f, 1.0f,
            0.0f, -1.0f, 0.0f, 1.0f,
            0.0f, 0.0f, -1.0f, 1.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        )
    }

    private var mEGLDisplay: EGLDisplay? = null
    private var mEGLConfig: EGLConfig? = null
    private var mEGLContext: EGLContext? = null
    private var mEGLSurface: EGLSurface? = null
    private var mProgram = 0
    private var mPositionId = 0
    private var mCoordId = 0
    private var mGLTextureId = -1
    private var mTexPreviewId = 0
    private var mTransformMatrixId = 0
    fun init(surface: SurfaceTexture, width: Int, height: Int) {
        initEGL(surface)
        initOpenGL(width, height)
    }

    private fun initOpenGL(width: Int, height: Int) {
        GLES20.glClearColor(0f, 0f, 0f, 1.0f)
        GLES20.glViewport(0, 0, width, height)

        val vertexShader = EglUtil.loadShader(VERTEX_SHADER, GLES20.GL_VERTEX_SHADER)
        val fragmentShader = EglUtil.loadShader(FRAGMENT_SHADER, GLES20.GL_FRAGMENT_SHADER)
        mProgram = EglUtil.createProgram(vertexShader, fragmentShader)
        GLES20.glDeleteShader(vertexShader)
        GLES20.glDeleteShader(fragmentShader)
        GLES20.glUseProgram(mProgram)

        mPositionId = GLES20.glGetAttribLocation(mProgram, "aPosition")
        GLES20.glEnableVertexAttribArray(mPositionId)
        GLES20.glVertexAttribPointer(mPositionId, 2, GLES20.GL_FLOAT, false, 0, EglUtil.mVerticesCoordinate)
        mCoordId = GLES20.glGetAttribLocation(mProgram, "aTextureCoord")
        GLES20.glEnableVertexAttribArray(mCoordId)
        GLES20.glVertexAttribPointer(mCoordId, 2, GLES20.GL_FLOAT, false, 0, EglUtil.mTextureInvertCoordinate)

        val mColorMatrixId = GLES20.glGetUniformLocation(mProgram, "uColorMatrix")
        GLES20.glUniformMatrix4fv(mColorMatrixId, 1, true, COLOR_MATRIX, 0);

        mTexPreviewId = GLES20.glGetUniformLocation(mProgram, "sTexture")
        mTransformMatrixId = GLES20.glGetUniformLocation(mProgram, "matTransform")
    }

    fun render(matrix: FloatArray, eglSurface: EGLSurface?) {
        makeCurrent(eglSurface)
        GLES20.glUniformMatrix4fv(mTransformMatrixId, 1, false, matrix, 0)
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0)
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, mGLTextureId)
        GLES20.glUniform1i(mTexPreviewId, 0)

        GLES20.glClear(GLES20.GL_DEPTH_BUFFER_BIT or GLES20.GL_COLOR_BUFFER_BIT)
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4)
        EGL14.eglSwapBuffers(mEGLDisplay, eglSurface)
    }

    private fun initEGL(surface: SurfaceTexture) {
        mEGLDisplay = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY)
        if (mEGLDisplay === EGL14.EGL_NO_DISPLAY) {
            throw RuntimeException("can't get eglGetDisplay")
        }
        if (!EGL14.eglInitialize(mEGLDisplay, null, 0, null, 0)) {
            throw RuntimeException("eglInitialize failed")
        }
        mEGLConfig = chooseEglConfig(mEGLDisplay)
        mEGLContext = createEglContext(mEGLDisplay, mEGLConfig)
        if (mEGLContext === EGL14.EGL_NO_CONTEXT) {
            throw RuntimeException("eglCreateContext failed")
        }
        mEGLSurface = createEGLSurface(Surface(surface))
        makeCurrent(mEGLSurface)
    }

    val defaultEGLSurface: EGLSurface?
        get() = mEGLSurface
    val texture: Int
        get() {
            if (mGLTextureId == -1) {
                val textures = IntArray(1)
                GLES20.glGenTextures(1, textures, 0)
                mGLTextureId = textures[0]
            }
            return mGLTextureId
        }

    private fun chooseEglConfig(display: EGLDisplay?): EGLConfig? {
        val attribList = intArrayOf(
            EGL14.EGL_BUFFER_SIZE, 32,
            EGL14.EGL_ALPHA_SIZE, 8,
            EGL14.EGL_RED_SIZE, 8,
            EGL14.EGL_GREEN_SIZE, 8,
            EGL14.EGL_BLUE_SIZE, 8,
            EGL14.EGL_RENDERABLE_TYPE, EGL14.EGL_OPENGL_ES2_BIT,
            EGL14.EGL_SURFACE_TYPE, EGL14.EGL_WINDOW_BIT,
            EGL14.EGL_NONE
        )
        val configs: Array<EGLConfig?> = arrayOfNulls(1)
        val numConfigs = IntArray(1)
        if (!EGL14.eglChooseConfig(
                display,
                attribList,
                0,
                configs,
                0,
                configs.size,
                numConfigs,
                0
            )
        ) {
            throw RuntimeException("eglChooseConfig failed")
        }
        return configs[0]
    }

    private fun createEglContext(display: EGLDisplay?, config: EGLConfig?): EGLContext {
        val contextList = intArrayOf(
            EGL14.EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL14.EGL_NONE
        )
        return EGL14.eglCreateContext(
            display,
            config,
            EGL14.EGL_NO_CONTEXT,
            contextList,
            0
        )
    }

    fun createEGLSurface(surface: Surface?): EGLSurface {
        val attribList = intArrayOf(
            EGL14.EGL_NONE
        )
        return EGL14.eglCreateWindowSurface(
            mEGLDisplay,
            mEGLConfig,
            surface,
            attribList,
            0
        )
    }

    private fun makeCurrent(eglSurface: EGLSurface?) {
        EGL14.eglMakeCurrent(mEGLDisplay, eglSurface, eglSurface, mEGLContext)
    }

    fun setPresentationTime(eglSurface: EGLSurface?, nsecs: Long) {
        EGLExt.eglPresentationTimeANDROID(mEGLDisplay, eglSurface, nsecs)
    }

    fun destroyEGLSurface(surface: EGLSurface?) {
        EGL14.eglDestroySurface(mEGLDisplay, surface)
    }

    fun deleteProgram(program: Int) {
        GLES20.glDeleteProgram(program)
    }
}