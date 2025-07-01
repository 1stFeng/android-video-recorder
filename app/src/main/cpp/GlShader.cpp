//
// Created by MOSI000211 on 2025/6/6.
//

#include "GlShader.h"
#include <GLES3/gl3.h>
#include <android/log.h>

#define TAG "GlShader"

extern unsigned int glCheckError_(const char* file, int line);
#define glCheckError() glCheckError_(__FILE__, __LINE__)

GlShader::GlShader(const char* vertex, const char* fragment)
{
    m_programId = 0;
    init_shader(vertex, fragment);
}

int GlShader::getAttribLocation(const std::string& name)
{
    if (mAttribTable.find(name) != mAttribTable.end())
    {
        return mAttribTable[name];
    }

    auto location = glGetAttribLocation(m_programId, name.c_str());
    if (location < 0)
    {
        __android_log_print(ANDROID_LOG_ERROR, TAG,"ERROR::SHADER::getAttribLocation: %s", name.c_str());
    }

    mAttribTable[name] = location;
    return location;
}

void GlShader::init_shader(const char* vertex, const char* fragment)
{
    /* 创建顶点着色器对象，并编译顶点着色器代码 */
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertex, nullptr);
    glCompileShader(vertexShader);
    glCheckError();

    int  success;
    char infoLog[512];
    /* 获取着色器编译结果 */
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        __android_log_print(ANDROID_LOG_ERROR, TAG,"ERROR::SHADER::VERTEX::COMPILATION_FAILED: %s", infoLog);
    }

    /* 创建片段着色器对象，并编译片段着色器代码 */
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragment, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    glCheckError();
    if (!success)
    {
        memset(infoLog, 0, sizeof(infoLog));
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        __android_log_print(ANDROID_LOG_ERROR, TAG,"ERROR::SHADER::FRAGMENT::COMPILATION_FAILED: %s", infoLog);
    }

    /* 创建片段着色器程序对象，并将着色器添加程序对象当中 */
    m_programId = glCreateProgram();
    glCheckError();
    glAttachShader(m_programId, vertexShader);
    glAttachShader(m_programId, fragmentShader);
    glLinkProgram(m_programId);
    glGetProgramiv(m_programId, GL_LINK_STATUS, &success);
    if (!success) {
        memset(infoLog, 0, sizeof(infoLog));
        glGetProgramInfoLog(m_programId, 512, nullptr, infoLog);
        __android_log_print(ANDROID_LOG_ERROR, TAG,"ERROR::PROGRAM::LINK_FAILED: %s", infoLog);
    }

    // 删除着色器，它们已经链接到我们的程序中了，已经不再需要了
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void GlShader::unBind() const
{
    glUseProgram(0);
}

void GlShader::bind() const
{
    glUseProgram(m_programId);
}

void GlShader::setMatrix4fv(const std::string &name, int size, bool transpose,const float *value) const
{
    auto location = glGetUniformLocation(m_programId, name.c_str());
    if (location < 0)
    {
        __android_log_print(ANDROID_LOG_ERROR, TAG,"ERROR::SHADER::setMatrix4fv: %d", location);
    }
    glUniformMatrix4fv(location, size, transpose, value);
}

void GlShader::setInt(const std::string &name, int value) const
{
    glUniform1i(glGetUniformLocation(m_programId, name.c_str()), value);
}
