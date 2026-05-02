#pragma once

#include <filesystem>
#include <stdexcept>

#include <opencv2/opencv.hpp>
#include <GL/glew.h>
#include <glm/glm.hpp>

#include "non_copyable.hpp"

class Texture : private NonCopyable {
public:
    enum class Interpolation {
        nearest,
        linear,
        linear_mipmap_linear,
    };

    Texture() = delete;
    Texture(const cv::Mat& image, Interpolation interpolation = Interpolation::linear_mipmap_linear);
    Texture(const std::filesystem::path& path, Interpolation interpolation = Interpolation::linear_mipmap_linear);
    Texture(const glm::vec3& color); // solid color RGB texture (1x1 pixel)
    Texture(const glm::vec4& color); // solid color RGBA texture (1x1 pixel)

    ~Texture();

    void bind(GLuint texture_unit = 0);
    GLuint get_name() const { return name_; }

private:
    GLuint name_{0};

    static cv::Mat load_image(const std::filesystem::path& path);
    void upload(const cv::Mat& image);
    void set_interpolation(Interpolation interpolation);
};
