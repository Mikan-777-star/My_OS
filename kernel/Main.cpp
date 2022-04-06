#include <cstdint>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"

void* operator new(unsigned long size, void* buf){
    return buf;
}
void operator delete(void* obj){};




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
    WriteString(pixel_writer, 1, 1, "Hello Ameux!!!");
    while (1)
    {
        __asm__("hlt");
    }
    
}