#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Uefi/UefiSpec.h>
#include  <Library/UefiBootServicesTableLib.h>
#include <Guid/FileInfo.h>

#include "Test.h"

EFI_STATUS GetMemoryMap(struct MemoryMap* map){
    if(map->buffer == NULL){
        return EFI_BUFFER_TOO_SMALL;
    }
    map->map_size = map->buffer_size;
    return gBS->GetMemoryMap(&map->map_size, (EFI_MEMORY_DESCRIPTOR*)map->buffer, &map->map_key, &map->descriptor_size, &map->descriptor_version);
}


const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type) {
  switch (type) {
    case EfiReservedMemoryType: return L"EfiReservedMemoryType";
    case EfiLoaderCode: return L"EfiLoaderCode";
    case EfiLoaderData: return L"EfiLoaderData";
    case EfiBootServicesCode: return L"EfiBootServicesCode";
    case EfiBootServicesData: return L"EfiBootServicesData";
    case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
    case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
    case EfiConventionalMemory: return L"EfiConventionalMemory";
    case EfiUnusableMemory: return L"EfiUnusableMemory";
    case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
    case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
    case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
    case EfiPalCode: return L"EfiPalCode";
    case EfiPersistentMemory: return L"EfiPersistentMemory";
    case EfiMaxMemoryType: return L"EfiMaxMemoryType";
    default: return L"InvalidMemoryType";
  }
}

EFI_STATUS SaveMemoryMap(struct MemoryMap* memmap, EFI_FILE_PROTOCOL* file){
    char buf[256];
    UINTN len;
    char* header = "Index, Type, Type(name), PhysicalStart, NumberofPages, Attribute";
    len = AsciiStrLen(header);
    file->Write(file, &len, header);
    Print(L"map->buffer = %08lx, map->map_size = %08lx\n", memmap->buffer_size, memmap->map_size);
    EFI_PHYSICAL_ADDRESS iter;
    int i;
    for(iter = (EFI_PHYSICAL_ADDRESS)memmap->buffer , i = 0; iter < (EFI_PHYSICAL_ADDRESS)memmap->buffer + memmap->map_size; iter += memmap->descriptor_size){
        EFI_MEMORY_DESCRIPTOR* desc  = (EFI_MEMORY_DESCRIPTOR*)iter;
        len = AsciiPrint(buf, sizeof(buf), "%u, %x, %-ls, %08lx, %lx, %lx\n",
        i, desc->Type, GetMemoryTypeUnicode(desc->Type),
        desc->PhysicalStart, desc->NumberOfPages,
        desc->Attribute & 0xffffflu);
        file->Write(file, &len, buf);
    }
    file->Flush(file);
    return EFI_SUCCESS;
    
}

EFI_STATUS open_file(EFI_HANDLE* image_handle, EFI_FILE_PROTOCOL** memmap_file, CHAR8* file){
    EFI_FILE_PROTOCOL* root_dir;
    OpenRootDir(*image_handle, &root_dir);
    root_dir->Open(root_dir, memmap_file, file, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE, 0);
    return 0;
}

EFI_STATUS OpenGOP(EFI_HANDLE image_handle,
                   EFI_GRAPHICS_OUTPUT_PROTOCOL** gop) {
  UINTN num_gop_handles = 0;
  EFI_HANDLE* gop_handles = NULL;
  gBS->LocateHandleBuffer(
      ByProtocol,
      &gEfiGraphicsOutputProtocolGuid,
      NULL,
      &num_gop_handles,
      &gop_handles);

  gBS->OpenProtocol(
      gop_handles[0],
      &gEfiGraphicsOutputProtocolGuid,
      (VOID**)gop,
      image_handle,
      NULL,
      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  FreePool(gop_handles);

  return EFI_SUCCESS;
}

void Halt(void){
    while(1)__asm__("hlt");
}

EFI_STATUS EFIAPI UefiMain(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table){
    Print(L"Hello world!!!\n");
    EFI_STATUS status = EFI_SUCCESS;

    //メモリの取得　根幹を握ることになるシステムだし、しっかり書こうね
    char* memmap_buf[4096 * 4];
    struct MemoryMap memmap = {sizeof(memmap_buf), (VOID*)memmap_buf, 0, 0, 0, 0};
    status = GetMemoryMap(&memmap);
    if(EFI_ERROR(status)){
        Print("faild to GetMemoryMap: %r\n", status);
        Halt();
    }
    EFI_FILE_PROTOCOL* memmap_file;
    status = open_file(&image_handle, &memmap_file, L"memmap");
    if(EFI_ERROR(status)){
        Print("faild to Open File\n");
        Halt();
    }
    SaveMemoryMap(&memmap, memmap_file);

    //カーネルファイルの読み込み
    EFI_FILE_PROTOCOL* kernel_file;
    status = open_file(image_handle, &kernel_file, "kernel.o");
    if(EFI_ERROR(status)){
        Print("faild to Open File\n");
        Halt();
    }
    //ファイルサイズの取得。　メモリをどれくらい使うかとかを調べる
    UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
    UINT8 file_info_buffer[file_info_size];
    kernel_file->GetInfo(kernel_file, &gEfiFileInfoGuid, &file_info_size, file_info_buffer);
    EFI_FILE_INFO* file_info = (EFI_FILE_INFO*) file_info_buffer;
    UINTN kernel_file_size = file_info->FileSize;
    //メモリの取得　そして読み込み
    EFI_PHYSICAL_ADDRESS kernel_base_addr = 0x100000;
    status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, (kernel_file_size + 0xfff) / 0x1000, &kernel_base_addr);
    if(EFI_ERROR(status)){
        Print("faild to Allocate pages:%r\n", status);
        Halt();
    }
    kernel_file->Read(kernel_file, &kernel_file_size, (void*)kernel_base_addr);
    Print("kernel File loaded (memory: 0x%0lx, %lubytes)\n",kernel_base_addr, kernel_file_size);

    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
    OpenGOP(image_handle, &gop);

    //bootサービスのお片付け　片付けできなかったら止まる（なんで片付けなければいけないのか　それがわからない）
    status = gBS->ExitBootServices(image_handle, memmap.map_key);
    if(EFI_ERROR(status)){
        status = GetMemoryMap(&memmap);
        if(EFI_ERROR(status)){
            Print("faild to memory_key %r\n", status);
            while(1)__asm__("hlt");
        }
        status = gBS->ExitBootServices(image_handle, memmap.map_key);
        if(EFI_ERROR(status)){
            Print("Could not exit boot service %r\n ...I don't know why you shouldn't use it\\(^_^)/", status);
            while(1)__asm__("hlt");
        }
    }
    //カーネル実行　ただの関数だけどね…
    UINT64 kernel_function_addr = *(UINT64*)(kernel_base_addr + 24);
    typedef void KernelMain(UINT64, UINT64);
    KernelMain* kernel = (KernelMain*)kernel_function_addr;
    kernel(gop->Mode->FrameBufferBase, gop->Mode->FrameBufferSize);
    while (1)__asm__("hlt");
    return 0;
}
