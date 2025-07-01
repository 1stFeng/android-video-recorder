//
// Created by MOSI000211 on 2025/6/6.
//

#ifndef CAMERAPREVIEW_GLSHADERSTRING_H
#define CAMERAPREVIEW_GLSHADERSTRING_H

static const char* DefaultVertexShader = R"(#version 330 core
attribute vec3 aPos;
attribute vec2 inputTextureCoordinate;
varying highp vec2 textureCoordinate;
void main()
{
    gl_Position = vec4(aPos.x , aPos.y, aPos.z, 1.0);
    textureCoordinate = inputTextureCoordinate.xy;
})";

static const char* DefaultFragmentShader = R"(#version 330 core
out vec4 FragColor;
varying highp vec2 textureCoordinate;
uniform sampler2D inputImageTexture;
void main()
{
    FragColor = texture2D(inputImageTexture, textureCoordinate);
})";

static const char* NV12VertexShader = R"(attribute vec4 aPosition;
attribute vec4 aTextureCoord;
varying vec2 vTextureCoord;
void main()
{
    gl_Position = aPosition;
    vTextureCoord = aTextureCoord.xy;
})";

static const char* NV12FragmentShader = R"(precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D   tex2D_y;
uniform sampler2D   tex2D_uv;

const mediump mat4 mat_vec_yuv2rgb = mat4
        (
         1.00000,  1.00000,  1.00000, 0
         ,        0, -0.34550,  1.77900, 0
         ,  1.40750, -0.71690,        0, 0
         , -0.70375,  0.53120, -0.88950, 0
         );
void main()
{
     mediump vec4 yuvColor;
     yuvColor.r = texture2D(tex2D_y, vTextureCoord).r;
     yuvColor.gb = texture2D(tex2D_uv, vTextureCoord).ra;
     yuvColor.a = 1.0;

     mediump vec4 rgbColor = mat_vec_yuv2rgb * yuvColor;
     gl_FragColor = vec4(rgbColor.r, rgbColor.g, rgbColor.b, 1.0);
})";

#endif //CAMERAPREVIEW_GLSHADERSTRING_H
