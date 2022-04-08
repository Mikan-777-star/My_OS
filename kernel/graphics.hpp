#include "frame_buffer_config.hpp"

#include <cstdint>

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
    const struct FrameBuffer_config* getConf();
    uint8_t* PixelAt(int x, int y);
};

class RGBPixelWriter : public PixelWriter{
    public:
    using PixelWriter::PixelWriter;
    virtual void Write(int x, int y, const struct Pixel_Color* c) override;
};

class BGRPixelWriter : public PixelWriter{
    public:
    using PixelWriter::PixelWriter;
    virtual void Write(int x, int y, const struct Pixel_Color* c) override;
};