//
// Created by MOSI000211 on 2025/6/6.
//

#ifndef CAMERAPREVIEW_GLSHADER_H
#define CAMERAPREVIEW_GLSHADER_H

#include <string>
#include <unordered_map>

class GlShader {
public:
    GlShader(const char* vertex, const char* fragment);
    int getAttribLocation(const std::string& name);
    void bind() const;
    void unBind() const;

    /**
     *
     * @param name uniform参数名称
     * @param size 矩阵个数
     * @param transpose 行列是否交换
     * @param value 矩阵列主序值
     */
    void setMatrix4fv(const std::string& name, int size, bool transpose, const float* value) const;
    void setInt(const std::string& name, int value) const;

private:
    void init_shader(const char* vertex, const char* fragment);

private:
    // 程序ID
    unsigned int m_programId;
    // 存放uniform的Location
    std::unordered_map<std::string, int> mAttribTable;
};

#endif //CAMERAPREVIEW_GLSHADER_H
