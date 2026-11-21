#include "core/Screenshot.h"

#include <glad/glad.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

namespace screenshot {

bool saveFramebuffer(const std::string& path, int width, int height) {
    if (width <= 0 || height <= 0) {
        std::cerr << "[Screenshot] bad framebuffer size.\n";
        return false;
    }

    const std::size_t pixelCount = static_cast<std::size_t>(width) *
                                   static_cast<std::size_t>(height);
    std::vector<unsigned char> pixels(pixelCount * 3);

    // glReadPixels defaults to 4-byte row alignment, which corrupts the image
    // whenever width*3 is not a multiple of 4. Ask for tight packing instead.
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "[Screenshot] could not open " << path << " for writing.\n";
        return false;
    }

    // PPM header: magic, dimensions, max channel value.
    file << "P6\n" << width << ' ' << height << "\n255\n";

    // OpenGL hands back rows bottom-to-top; image files expect top-to-bottom,
    // so write the rows out in reverse.
    const std::size_t rowBytes = static_cast<std::size_t>(width) * 3;
    for (int row = height - 1; row >= 0; --row) {
        file.write(reinterpret_cast<const char*>(
                       pixels.data() + static_cast<std::size_t>(row) * rowBytes),
                   static_cast<std::streamsize>(rowBytes));
    }

    if (!file) {
        std::cerr << "[Screenshot] failed while writing " << path << ".\n";
        return false;
    }

    std::cout << "[Screenshot] wrote " << path << " (" << width << "x" << height << ")\n";
    return true;
}

} // namespace screenshot
