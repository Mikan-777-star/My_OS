#include "Console.hpp"

#include <string.h>

Console::Console(PixelWriter* pw){
    _pw = pw;
    nowX = 1;
    nowY = 1;
}

int Console::WriteString(const char* str){
    for(int i = 0; str[i] != '\0'; i++){
        write_char(_pw, nowX, nowY, str[i]);
        nowX += 8;
    }
}

int Console::NewLine(){
    if(nowY + 16 > _pw->getConf()->vertical_resolution - 1){
        char buf[_pw->getConf()->horizontal_resolution * 3];
        for(int i = 17; i < _pw->getConf()->vertical_resolution; i++){
            memcpy(buf, _pw->PixelAt(0, i), sizeof(buf));
            memcpy(_pw->PixelAt(0, i - 16), buf, sizeof(buf));
        }
    }else{
        nowY += 16;
        nowX = 1;
    }
}