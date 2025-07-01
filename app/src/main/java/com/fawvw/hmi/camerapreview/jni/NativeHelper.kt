/******************************************************************************
 * China Entry Infotainment Project
 * Copyright (c) 2023 FAW-VW, P-VC & MOSI-Tech.
 * All Rights Reserved.
 *******************************************************************************/

/******************************************************************************
 * @file nativeHelper.kt
 * @ingroup  CameraPreview
 * @author fenghuaping
 * @brief This file is designed as xxxx service
 ******************************************************************************/
package com.fawvw.hmi.camerapreview.jni

import android.view.Surface

object NativeHelper {
    init {
        System.loadLibrary("native-lib")
    }

    external fun nativeSetPreviewSurface(surface: Surface)
    external fun nativeStartRecord(surface: Surface)
    external fun nativeStopRecord()
    external fun nativeStartPreview()
    external fun nativeStopPreview()
    external fun nativeStartEncoder()
    external fun nativePrepareEncoder(path: String)

}