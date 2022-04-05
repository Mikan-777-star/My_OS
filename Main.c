#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Uefi/UefiSpec.h>
#include  <Library/UefiBootServicesTableLib.h>
#include <Guid/FileInfo.h>
#include <Protocol/LoadedImage.h>

#include "Test.h"
#include "kernel/frame_buffer_config.hpp"
#include "kernel/elf.hpp"

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

EFI_STATUS OpenRootDir(EFI_HANDLE* image_handle, EFI_FILE_PROTOCOL** root){
    EFI_STATUS status;
    EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

    status = gBS->OpenProtocol(
        *image_handle,
        &gEfiLoadedImageProtocolGuid,
        (void**)&loaded_image,
        *image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
    );
    if(EFI_ERROR(status)){
        return status;
    }

    return fs->OpenVolume(fs, root);
}


EFI_STATUS open_file(EFI_FILE_PROTOCOL* root_dir, EFI_FILE_PROTOCOL** memmap_file, CHAR8* file){
    return root_dir->Open(root_dir, memmap_file, file, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE, 0);
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


void CalcLoadAddressRange(Elf64_Ehdr* ehdr, UINT64* first, UINT64* last) {
  Elf64_Phdr* phdr = (Elf64_Phdr*)((UINT64)ehdr + ehdr->e_phoff);
  *first = MAX_UINT64;
  *last = 0;
  for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i) {
    if (phdr[i].p_type != PT_LOAD) continue;
    *first = MIN(*first, phdr[i].p_vaddr);
    *last = MAX(*last, phdr[i].p_vaddr + phdr[i].p_memsz);
  }
}

void CopyLoadSegments(Elf64_Ehdr* ehdr) {
  Elf64_Phdr* phdr = (Elf64_Phdr*)((UINT64)ehdr + ehdr->e_phoff);
  for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i) {
    if (phdr[i].p_type != PT_LOAD) continue;

    UINT64 segm_in_file = (UINT64)ehdr + phdr[i].p_offset;
    CopyMem((VOID*)phdr[i].p_vaddr, (VOID*)segm_in_file, phdr[i].p_filesz);

    UINTN remain_bytes = phdr[i].p_memsz - phdr[i].p_filesz;
    SetMem((VOID*)(phdr[i].p_vaddr + phdr[i].p_filesz), remain_bytes, 0);
  }
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
    EFI_FILE_PROTOCOL* root_dir;
    OpenRootDir(&image_handle, &root_dir);


    EFI_FILE_PROTOCOL* memmap_file;
    status = open_file(root_dir, &memmap_file, L"memmap");
    if(EFI_ERROR(status)){
        Print("faild to Open File\n");
        Halt();
    }
    SaveMemoryMap(&memmap, memmap_file);



    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
    OpenGOP(image_handle, &gop);

    //カーネルファイルの読み込み
    EFI_FILE_PROTOCOL* kernel_file;
    status = open_file(root_dir, &kernel_file, L"kernel.o");
    if(EFI_ERROR(status)){
        Print(L"faild to Open File\n");
        Halt();
    }

    UINTN file_info_size = sizeof(EFI_FILE_INFO )+ sizeof(CHAR16) * 12;
    UINT8 file_info_buffer[file_info_size];
    status = kernel_file->GetInfo(kernel_file, &gEfiFileInfoGuid, &file_info_size, file_info_buffer);
    if(EFI_ERROR(status)){
        Print(L"faild to get file info: %r\n", status);
        Halt();
    }
    EFI_FILE_INFO* file_info = (EFI_FILE_INFO*)file_info_buffer;
    UINTN kernel_file_size = file_info->FileSize;

    void* kernel_buffer;
    status = gBS->AllocatePool(EfiLoaderData, kernel_file_size, &kernel_buffer);
    if(EFI_ERROR(status)){
        Print(L"faild to allocate pool: %r\n", status);
        Halt();
    }
    status = kernel_file->Read(kernel_file, &kernel_file_size, kernel_buffer);
    if(EFI_ERROR(status)){
        Print(L"Error: %r\n", status);
        Halt();
    }
    Elf64_Ehdr* kernel_ehdr = (Elf64_Ehdr*)kernel_buffer;
    UINT64 kernel_first_addr, kernel_last_addr;
    CalcLoadAddressRange(kernel_ehdr, &kernel_first_addr, &kernel_last_addr);
    UINTN num_pages =(kernel_last_addr - kernel_first_addr + 0xfff) / 0x1000;
    status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, num_pages, &kernel_first_addr);
    if(EFI_ERROR(status)){
        Print(L"faild to allocate pages: %r\n", status);
        Halt();
    }




    
    CopyLoadSegment(kernel_ehdr);
    Print(L"kernel 0x%0lx - 0x%0lx\n",kernel_first_addr, kernel_last_addr);
    status = gBS->FreePool(kernel_buffer);
    if(EFI_ERROR(status)){
        Print("faild to free pool%r\n", status);
        Halt();
    }
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

    UINT64 entry_addr = *(UINT64*)(kernel_first_addr + 24);

    struct FrameBuffer_config conf = {
        (char*)gop->Mode->FrameBufferBase,
        gop->Mode->Info->PixelsPerScanLine,
        gop->Mode->Info->HorizontalResolution,
        gop->Mode->Info->VerticalResolution,
        0 
    };
    switch (gop->Mode->Info->PixelFormat)
    {
    case PixelRedGreenBlueReserved8BitPerColor:
        conf.pixel_format = PixelRGB8bitPerColor;
        break;
    case PixelBlueGreenRedReserved8BitPerColor:
        conf.pixel_format = PixelBGR8bitPerColor;
        break;
    default:
        Print("Unimplemented pixel format: %d\n nanikore!\\(^o^)/\n", gop->Mode->Info->PixelFormat);
        Halt();
        break;
    }

    typedef void kernel(UINT64 FrameBuffer_config_point);
    kernel* kernel_entry = (kernel*)entry_addr;
    kernel_entry((UINT64)&conf);
    
    while (1)__asm__("hlt");
    return 0;
}
