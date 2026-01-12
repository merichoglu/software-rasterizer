#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "texture.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>

Texture::Texture() :
    width(0),
    height(0),
    channels(4),
    filter_mode(FilterMode::BILINEAR),
    wrap_mode(WrapMode::REPEAT)
{}

float Texture::wrap_coord(float coord) const {
    switch (wrap_mode) {
        case WrapMode::REPEAT:
            coord = coord - std::floor(coord);
            break;
        case WrapMode::CLAMP_TO_EDGE:
            coord = std::clamp(coord, 0.0f, 1.0f);
            break;
        case WrapMode::MIRRORED_REPEAT: {
            float integer_part;
            coord = std::modf(coord, &integer_part);
            if (coord < 0.0f) coord += 1.0f;
            int period = static_cast<int>(std::abs(integer_part));
            if (period % 2 == 1) {
                coord = 1.0f - coord;
            }
            break;
        }
    }
    return coord;
}

Color Texture::get_pixel(int x, int y) const {
    x = std::clamp(x, 0, width - 1);
    y = std::clamp(y, 0, height - 1);
    return pixels[y * width + x];
}

bool Texture::load(const std::string& filepath) {
    /* determine file type from extension */
    size_t dot_pos = filepath.find_last_of('.');
    if (dot_pos == std::string::npos) {
        std::cerr << "Texture: Unknown file format (no extension): " << filepath << std::endl;
        return false;
    }

    std::string ext = filepath.substr(dot_pos + 1);
    for (auto& c : ext) c = static_cast<char>(std::tolower(c));

    if (ext == "tga") {
        /* load TGA file */
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Texture: Failed to open file: " << filepath << std::endl;
            return false;
        }

        /* TGA header */
        unsigned char header[18];
        file.read(reinterpret_cast<char*>(header), 18);

        unsigned char id_length = header[0];
        unsigned char color_map_type = header[1];
        unsigned char image_type = header[2];
        width = header[12] | (header[13] << 8);
        height = header[14] | (header[15] << 8);
        unsigned char bits_per_pixel = header[16];

        /* skip unsupported formats */
        if (color_map_type != 0) {
            std::cerr << "Texture: Color-mapped TGA not supported" << std::endl;
            return false;
        }

        if (image_type != 2 && image_type != 3) {
            std::cerr << "Texture: Only uncompressed TGA supported (type 2 or 3)" << std::endl;
            return false;
        }

        /* skip image ID */
        if (id_length > 0) {
            file.seekg(id_length, std::ios::cur);
        }

        channels = bits_per_pixel / 8;
        pixels.resize(width * height);

        /* read pixel data */
        for (int y = 0; y < height; ++y) {
            /* TGA stores bottom-to-top by default */
            int row = height - 1 - y;
            for (int x = 0; x < width; ++x) {
                unsigned char pixel[4] = {255, 255, 255, 255};
                file.read(reinterpret_cast<char*>(pixel), channels);

                /* TGA stores BGR(A) */
                float b = pixel[0] / 255.0f;
                float g = (channels > 1) ? pixel[1] / 255.0f : b;
                float r = (channels > 2) ? pixel[2] / 255.0f : b;
                float a = (channels > 3) ? pixel[3] / 255.0f : 1.0f;

                pixels[row * width + x] = Color(r, g, b, a);
            }
        }

        file.close();
        std::cout << "Loaded texture: " << filepath << " (" << width << "x" << height << ")" << std::endl;
        return true;
    }
    else if (ext == "ppm") {
        /* load PPM file (P6 binary format) */
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Texture: Failed to open file: " << filepath << std::endl;
            return false;
        }

        std::string magic;
        file >> magic;
        if (magic != "P6") {
            std::cerr << "Texture: Only P6 PPM format supported" << std::endl;
            return false;
        }

        /* skip comments */
        char c;
        file.get(c);
        while (file.peek() == '#') {
            std::string comment;
            std::getline(file, comment);
        }

        int max_val;
        file >> width >> height >> max_val;
        file.get(c); /* skip single whitespace after max_val */

        channels = 3;
        pixels.resize(width * height);

        /* read pixel data */
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                unsigned char rgb[3];
                file.read(reinterpret_cast<char*>(rgb), 3);

                float r = rgb[0] / static_cast<float>(max_val);
                float g = rgb[1] / static_cast<float>(max_val);
                float b = rgb[2] / static_cast<float>(max_val);

                pixels[y * width + x] = Color(r, g, b, 1.0f);
            }
        }

        file.close();
        std::cout << "Loaded texture: " << filepath << " (" << width << "x" << height << ")" << std::endl;
        return true;
    }
    else if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "bmp") {
        /* load using stb_image */
        int w, h, c;
        unsigned char* data = stbi_load(filepath.c_str(), &w, &h, &c, 0);
        if (!data) {
            std::cerr << "Texture: Failed to load image: " << filepath << std::endl;
            std::cerr << "  stb_image error: " << stbi_failure_reason() << std::endl;
            return false;
        }

        width = w;
        height = h;
        channels = c;
        pixels.resize(width * height);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * channels;
                float r = data[idx] / 255.0f;
                float g = (channels > 1) ? data[idx + 1] / 255.0f : r;
                float b = (channels > 2) ? data[idx + 2] / 255.0f : r;
                float a = (channels > 3) ? data[idx + 3] / 255.0f : 1.0f;

                pixels[y * width + x] = Color(r, g, b, a);
            }
        }

        stbi_image_free(data);
        std::cout << "Loaded texture: " << filepath << " (" << width << "x" << height << ")" << std::endl;
        return true;
    }
    else {
        std::cerr << "Texture: Unsupported format: " << ext << std::endl;
        return false;
    }
}

bool Texture::create(int i_width, int i_height, const std::vector<Color>& data) {
    if (data.size() != static_cast<size_t>(i_width * i_height)) {
        std::cerr << "Texture: Data size mismatch" << std::endl;
        return false;
    }

    width = i_width;
    height = i_height;
    pixels = data;
    return true;
}

void Texture::create_solid(int i_width, int i_height, Color color) {
    width = i_width;
    height = i_height;
    pixels.resize(width * height, color);
}

void Texture::create_checkerboard(int i_width, int i_height, int squares, Color color1, Color color2) {
    width = i_width;
    height = i_height;
    pixels.resize(width * height);

    int square_size_x = width / squares;
    int square_size_y = height / squares;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int check_x = x / square_size_x;
            int check_y = y / square_size_y;
            bool is_color1 = ((check_x + check_y) % 2) == 0;
            pixels[y * width + x] = is_color1 ? color1 : color2;
        }
    }
}

Color Texture::sample(Vec2 uv) const {
    return sample(uv.x, uv.y);
}

Color Texture::sample(float u, float v) const {
    if (!is_valid()) {
        return Color(1.0f, 0.0f, 1.0f, 1.0f); /* magenta for missing texture */
    }

    /* wrap coordinates */
    u = wrap_coord(u);
    v = wrap_coord(v);

    /* flip V coordinate (OpenGL convention: 0 at bottom) */
    v = 1.0f - v;

    /* convert to pixel coordinates */
    float px = u * (width - 1);
    float py = v * (height - 1);

    if (filter_mode == FilterMode::NEAREST) {
        int x = static_cast<int>(std::round(px));
        int y = static_cast<int>(std::round(py));
        return get_pixel(x, y);
    }
    else { /* BILINEAR */
        int x0 = static_cast<int>(std::floor(px));
        int y0 = static_cast<int>(std::floor(py));
        int x1 = x0 + 1;
        int y1 = y0 + 1;

        float fx = px - x0;
        float fy = py - y0;

        Color c00 = get_pixel(x0, y0);
        Color c10 = get_pixel(x1, y0);
        Color c01 = get_pixel(x0, y1);
        Color c11 = get_pixel(x1, y1);

        /* bilinear interpolation */
        Color c0 = c00 * (1.0f - fx) + c10 * fx;
        Color c1 = c01 * (1.0f - fx) + c11 * fx;
        return c0 * (1.0f - fy) + c1 * fy;
    }
}

void Texture::set_filter_mode(FilterMode mode) {
    filter_mode = mode;
}

void Texture::set_wrap_mode(WrapMode mode) {
    wrap_mode = mode;
}

int Texture::get_width() const {
    return width;
}

int Texture::get_height() const {
    return height;
}

bool Texture::is_valid() const {
    return width > 0 && height > 0 && !pixels.empty();
}
