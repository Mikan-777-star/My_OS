#include "Console.hpp"

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
        
    }else{
        nowY += 16;
        nowX = 1;
    }
}