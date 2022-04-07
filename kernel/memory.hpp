#include <stdint.h>


struct MemoryMap{
    unsigned int buffer_size;
    void* buffer;
    unsigned int map_size;
    unsigned int map_key;
    unsigned int descriptor_size;
    uint32_t descriptor_version;
};