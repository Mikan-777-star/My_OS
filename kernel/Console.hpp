

#include "graphics.hpp"
#include "font.hpp"

class Console{
    private:
    int nowX, nowY;
    PixelWriter* _pw;
    public:
        Console(PixelWriter* pw);
        ~Console() = default;
        int WriteString(const char* str);
        int NewLine();
    
};