#include "graphics.hpp"

#include <cstdint>
uint8_t*  PixelWriter::PixelAt(int x, int y){
    return reinterpret_cast<uint8_t*>(_config->framebuffer + (4 * (_config->pixels_per_scan_line * y + x)));
}

void BGRPixelWriter::Write(int x, int y, const struct Pixel_Color* c) {
    auto p = PixelAt(x, y);
    p[2] = c->red;
    p[1] = c->green;
    p[0] = c->blue;
}

void RGBPixelWriter::Write(int x, int y, const struct Pixel_Color* c){
    auto p = PixelAt(x, y);
    p[0] = c->red;
    p[1] = c->green;
    p[2] = c->blue;
}