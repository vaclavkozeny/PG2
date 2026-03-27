#include "Texture.hpp"

// --------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------

cv::Mat Texture::load_image(const std::filesystem::path& path) {
    // cv::imread does NOT throw on missing file — must check manually
    cv::Mat img = cv::imread(path.string(), cv::IMREAD_UNCHANGED);
    if (img.empty())
        throw std::runtime_error("Texture: cannot load image: " + path.string());
    return img;
}

void Texture::upload(const cv::Mat& image) {
    // OpenGL origin is bottom-left; OpenCV origin is top-left — flip vertically
    cv::Mat flipped;
    cv::flip(image, flipped, 0);

    glBindTexture(GL_TEXTURE_2D, name_);

    switch (flipped.type()) {
    case CV_8UC1: // greyscale
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8,
                     flipped.cols, flipped.rows, 0,
                     GL_RED, GL_UNSIGNED_BYTE, flipped.data);
        // replicate single channel to G and B so it looks grey not red
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
        break;

    case CV_8UC3: // BGR (OpenCV default for colour images)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
                     flipped.cols, flipped.rows, 0,
                     GL_BGR, GL_UNSIGNED_BYTE, flipped.data);
        break;

    case CV_8UC4: // BGRA
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                     flipped.cols, flipped.rows, 0,
                     GL_BGRA, GL_UNSIGNED_BYTE, flipped.data);
        break;

    default:
        glBindTexture(GL_TEXTURE_2D, 0);
        throw std::runtime_error("Texture: unsupported image format (need 1/3/4 channel uint8)");
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::set_interpolation(Interpolation interpolation) {
    glBindTexture(GL_TEXTURE_2D, name_);

    // Texture wrapping — repeat by default
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    switch (interpolation) {
    case Interpolation::nearest:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        break;
    case Interpolation::linear:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        break;
    case Interpolation::linear_mipmap_linear:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D); // build mipmap chain
        break;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

// --------------------------------------------------------------------------
// Constructors
// --------------------------------------------------------------------------

Texture::Texture(const cv::Mat& image, Interpolation interpolation) {
    if (image.empty())
        throw std::runtime_error("Texture: input image is empty");

    glGenTextures(1, &name_);
    upload(image);
    set_interpolation(interpolation);
}

Texture::Texture(const std::filesystem::path& path, Interpolation interpolation)
    : Texture(load_image(path), interpolation) {}

Texture::Texture(const glm::vec3& color)
    : Texture(cv::Mat(1, 1, CV_8UC3,
              cv::Scalar(
                  static_cast<uint8_t>(color.b * 255),
                  static_cast<uint8_t>(color.g * 255),
                  static_cast<uint8_t>(color.r * 255))),
              Interpolation::nearest) {}

Texture::Texture(const glm::vec4& color)
    : Texture(cv::Mat(1, 1, CV_8UC4,
              cv::Scalar(
                  static_cast<uint8_t>(color.b * 255),
                  static_cast<uint8_t>(color.g * 255),
                  static_cast<uint8_t>(color.r * 255),
                  static_cast<uint8_t>(color.a * 255))),
              Interpolation::nearest) {}

// --------------------------------------------------------------------------
// Destructor / runtime
// --------------------------------------------------------------------------

Texture::~Texture() {
    if (name_ != 0)
        glDeleteTextures(1, &name_);
}

void Texture::bind(GLuint texture_unit) {
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(GL_TEXTURE_2D, name_);
}
