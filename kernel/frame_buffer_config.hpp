#pragma once

#include <stdint.h>

enum PixelFormat{
    PixelRGB8bitPerColor,
    PixelBGR8bitPerColor,
};

struct FrameBuffer_config{
    char* framebuffer;
    uint32_t pixels_per_scan_line;
    uint32_t horizontal_resolution;
    uint32_t vertical_resolution;
    enum PixelFormat pixel_format;
}__attribute__((packed));