#include <cstdint>

#include "frame_buffer_config.hpp"

void* operator new(unsigned long size, void* buf){
    return buf;
}
void operator delete(void* obj){};

struct Pixel_Color{
    uint8_t red, green, blue;
};

class PixelWriter
{
private:
    const struct FrameBuffer_config* _config;
public:
    PixelWriter(const struct FrameBuffer_config* config):_config{config}{};
    virtual ~PixelWriter() = default;
    virtual void Write(int x, int y, const struct Pixel_Color* c) = 0;
protected:
    uint8_t* PixelAt(int x, int y){
        return reinterpret_cast<uint8_t*>(_config->framebuffer + (4 * (_config->pixels_per_scan_line * y + x)));
    }
};

class RGBPixelWriter : public PixelWriter{
    public:
    using PixelWriter::PixelWriter;
    virtual void Write(int x, int y, const struct Pixel_Color* c) override{
        auto p = PixelAt(x, y);
        p[0] = c->red;
        p[1] = c->green;
        p[2] = c->blue;
    }
};

class BGRPixelWriter : public PixelWriter{
    public:
    using PixelWriter::PixelWriter;
    virtual void Write(int x, int y, const struct Pixel_Color* c) override{
        auto p = PixelAt(x, y);
        p[2] = c->red;
        p[1] = c->green;
        p[0] = c->blue;
    }
};

char pixel_writer_buf[sizeof(RGBPixelWriter)];
PixelWriter* pixel_writer;

extern "C" void kernelMain(uint64_t framebuffer_config_point)
{
    const struct FrameBuffer_config* conf = reinterpret_cast<const struct FrameBuffer_config*>(framebuffer_config_point);
    sizeof(conf);

    switch (conf->pixel_format)
    {
   case PixelBGR8bitPerColor:
       pixel_writer = new(pixel_writer_buf)BGRPixelWriter{conf};
       break;
   case PixelRGB8bitPerColor:
        pixel_writer = new(pixel_writer_buf)RGBPixelWriter{conf};
        break;   
    }
    
    while (1)
    {
        __asm__("hlt");
    }
    
}