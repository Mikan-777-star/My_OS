#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Uefi/UefiSpec.h>

struct MemoryMap{
    UINTN buffer_size;
    VOID* buffer;
    UINTN map_size;
    UINTN map_key;
    UINTN descriptor_size;
    UINT32 descriptor_version;
};