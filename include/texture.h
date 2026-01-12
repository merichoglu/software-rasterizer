#pragma once

#include "math/vector.h"
#include <vector>
#include <string>

/* texture filtering modes */
enum class FilterMode {
    NEAREST,    /* nearest neighbor (pixelated) */
    BILINEAR    /* bilinear interpolation (smooth) */
};

/* texture wrap modes */
enum class WrapMode {
    REPEAT,         /* tile the texture */
    CLAMP_TO_EDGE,  /* clamp to edge pixels */
    MIRRORED_REPEAT /* tile with mirroring */
};

class Texture {
    private:
        std::vector<Color> pixels;
        int width;
        int height;
        int channels;
        FilterMode filter_mode;
        WrapMode wrap_mode;

        /* wrap UV coordinate based on wrap mode */
        float wrap_coord(float coord) const;

        /* get pixel at integer coordinates (with bounds checking) */
        Color get_pixel(int x, int y) const;

    public:
        /* constructor */
        Texture();

        /* load texture from file (supports TGA, PPM) */
        bool load(const std::string& filepath);

        /* create texture from raw data */
        bool create(int i_width, int i_height, const std::vector<Color>& data);

        /* create solid color texture */
        void create_solid(int i_width, int i_height, Color color);

        /* create procedural checkerboard texture */
        void create_checkerboard(int i_width, int i_height, int squares, Color color1, Color color2);

        /* sample texture at UV coordinates */
        Color sample(Vec2 uv) const;
        Color sample(float u, float v) const;

        /* setters */
        void set_filter_mode(FilterMode mode);
        void set_wrap_mode(WrapMode mode);

        /* getters */
        int get_width() const;
        int get_height() const;
        bool is_valid() const;
};
