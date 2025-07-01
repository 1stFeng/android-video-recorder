/******************************************************************************
 * China Entry Infotainment Project
 * Copyright (c) 2023 FAW-VW, P-VC & MOSI-Tech.
 * All Rights Reserved.
 *******************************************************************************/

/******************************************************************************
 * @file EglUtil.kt
 * @ingroup  CameraPreview
 * @author fenghuaping
 * @brief This file is designed as xxxx service
 ******************************************************************************/
package com.fawvw.hmi.camerapreview.utils

import android.opengl.GLES20
import android.opengl.GLES30
import android.util.Log
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.FloatBuffer

object EglUtil {
    private const val TAG = "EglUtil"
    // 标准纹理坐标
    val mTextureCoordinate = allocDirectFloatBuffer(0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f)
    // 纹理翻转坐标
    val mTextureInvertCoordinate = allocDirectFloatBuffer(0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f)
    // 标准顶点坐标
    val mVerticesCoordinate = allocDirectFloatBuffer(-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f)

    private fun allocDirectFloatBuffer(vararg elements: Float): FloatBuffer {
        val floatBuffer = ByteBuffer.allocateDirect(elements.size * 4)
            .order(ByteOrder.nativeOrder())
            .asFloatBuffer()
            .put(elements)
        floatBuffer.position(0)
        return floatBuffer
    }

    fun loadShader(strSource: String?, iType: Int): Int {
        val compiled = IntArray(1)
        val iShader = GLES20.glCreateShader(iType)
        GLES20.glShaderSource(iShader, strSource)
        GLES20.glCompileShader(iShader)
        GLES20.glGetShaderiv(iShader, GLES20.GL_COMPILE_STATUS, compiled, 0)
        if (compiled[0] == 0) {
            Log.d(TAG, "shader compile failed: ${GLES20.glGetShaderInfoLog(iShader)}.")
            return 0
        }
        return iShader
    }

    fun createProgram(vertexShader: Int, fragmentShader: Int): Int {
        val program = GLES20.glCreateProgram()
        Log.d(TAG, "createProgram: ...program:$program")
        checkGlError("createProgram")
        if (program == 0) {
            throw RuntimeException("Could not create program")
        }
        GLES20.glAttachShader(program, vertexShader)
        checkGlError("createProgram() attach vertex Shader")
        GLES20.glAttachShader(program, fragmentShader)
        checkGlError("createProgram() attach fragment Shader")
        GLES20.glLinkProgram(program)
        val linkStatus = IntArray(1)
        GLES20.glGetProgramiv(program, GLES20.GL_LINK_STATUS, linkStatus, 0)
        checkGlError("glLinkProgram")
        if (linkStatus[0] != GLES20.GL_TRUE) {
            GLES20.glDeleteProgram(program)
            Log.e(
                TAG,
                "linkProgram: link failed log:" + GLES20.glGetProgramInfoLog(program) + ",linkStatus:" + linkStatus[0]
            )
            throw RuntimeException("Could not link program")
        }
        return program
    }

    private fun checkGlError(op: String) {
        val error = GLES30.glGetError()
        if (error != GLES20.GL_NO_ERROR) {
            val msg = op + "  0x" + Integer.toHexString(error)
            Log.e(TAG, "checkGlError: $msg")
        }
    }
}