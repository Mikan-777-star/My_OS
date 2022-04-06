#include <cstdint>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"

void* operator new(unsigned long size, void* buf){
    return buf;
}
void operator delete(void* obj){};




const uint8_t kFontA[16]{
    0b00000000,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00100100,
    0b00100100,
    0b00100100,
    0b00100100,
    0b01111110,
    0b01000010,
    0b01000010,
    0b01000010,
    0b11100111,
    0b00000000,
    0b00000000
};

char pixel_writer_buf[sizeof(RGBPixelWriter)];
PixelWriter* pixel_writer;

extern "C" void kernelMain(uint64_t framebuffer_config_point)
{
    const struct FrameBuffer_config* conf = reinterpret_cast<const struct FrameBuffer_config*>(framebuffer_config_point);

    switch (conf->pixel_format)
    {
        case PixelBGR8bitPerColor:
            pixel_writer = new(pixel_writer_buf)BGRPixelWriter{conf};
            break;
        case PixelRGB8bitPerColor:
            pixel_writer = new(pixel_writer_buf)RGBPixelWriter{conf};
            break;   
    }
    const struct Pixel_Color pixel = {10, 20, 254};
    for(int x = 10; x < 110; x++){
        for(int y = 10; y < 110; y++){
            pixel_writer->Write(x, y, &pixel);
        }
    }
    while (1)
    {
        __asm__("hlt");
    }
    
}