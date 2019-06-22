//
//  Kernel.c
//  Unrootless
//
//  Created by Linus Henze on 11/10/2016.
//  Copyright © 2016 Linus Henze. All rights reserved.
//

#include <kern/host.h>
#include "Kernel.h"
#include "Filesystem.h"

#define KERNEL_READ_SIZE 8192 // 0x2000 bytes should be enough

static char *kernel_paths[] = {
    "/System/Library/Kernels/kernel",
    "/System/Library/Kernels/kernel.development",
    "/System/Library/Kernels/kernel.debug"
};

struct KernelInfo kInfo;

mach_vm_address_t findKernelBase() {
    host_priv_t host = host_priv_self();
    mach_vm_address_t searchAddress = (mach_vm_address_t) host & 0xfffffffffff00000;
    while (searchAddress > 0) {
        if (*(uint32_t*)(searchAddress) == MH_MAGIC_64) {
            // Make sure this is really the Header
            // If it is, the __TEXT load command will follow
            struct segment_command_64 *__text = (struct segment_command_64*) (searchAddress + sizeof(struct mach_header_64));
            if (strncmp(__text->segname, "__TEXT", 16) == 0) {
                return searchAddress;
            }
        }
        searchAddress--;
    }
    return 0;
}

struct load_command *findLoadCommandOfType(uint8_t *kernel, struct load_command *begin, uint32_t type) {
    struct mach_header_64 *header = (struct mach_header_64*) kernel;
    if (header->magic != MH_MAGIC_64) {
        return NULL; //That's not a mach-o image
    }
    
    struct load_command *startCmd = (struct load_command*) (kernel + sizeof(struct mach_header_64));
    
    struct load_command *ldCmd = begin;
    if (ldCmd == NULL) {
        ldCmd = startCmd;
    } else {
        ldCmd = (struct load_command*) ((mach_vm_address_t) ldCmd + ldCmd->cmdsize);
    }
    
    mach_vm_address_t endAddr = (mach_vm_address_t) startCmd + header->sizeofcmds;
    
    while ((mach_vm_address_t) ldCmd < endAddr) {
        if (ldCmd->cmd == type) {
            return ldCmd;
        }
        ldCmd = (struct load_command*) ((mach_vm_address_t) ldCmd + ldCmd->cmdsize);
    }
    
    return NULL;
}

struct segment_command_64 *findSegmentLoadCommand(uint8_t *kernel, char *segment) {
    struct load_command *ldCmd = findLoadCommandOfType(kernel, NULL, LC_SEGMENT_64);
    
    while (ldCmd != NULL) {
        struct segment_command_64 *sLdCmd = (struct segment_command_64*) ldCmd;
        if (strncmp(sLdCmd->segname, segment, 16) == 0) {
            return (struct segment_command_64*) ldCmd;
        } else {
            ldCmd = findLoadCommandOfType(kernel, ldCmd, LC_SEGMENT_64);
        }
    }
    
    return NULL;
}

bool findUUID(uint8_t *kernel, uint8_t *buffer) {
    struct uuid_command *uuidCmd = (struct uuid_command*) findLoadCommandOfType(kernel, NULL, LC_UUID);
    if (uuidCmd == NULL) {
        return false;
    }
    
    memcpy(buffer, uuidCmd->uuid, sizeof(uint8_t) * 16);
    return true;
}

bool UUID_matches_running(uint8_t *kernelDisk) {
    uint8_t uuid[16];
    uint8_t uuidRunning[16];
    
    uint8_t *kernelRunning = (uint8_t*) findKernelBase();
    if (kernelRunning == NULL) {
        return false;
    }
    
    if (!findUUID(kernelDisk, uuid)) {
        return false;
    }
    
    if (!findUUID(kernelRunning, uuidRunning)) {
        return false;
    }
    
    if (memcmp(uuid, uuidRunning, sizeof(uint8_t) * 16) == 0) {
        return true;
    }
    
    return false;
}

bool initKernelInfo() {
    bool res = false;
    
    kInfo.kernelBase = (uint8_t*) findKernelBase();
    if (kInfo.kernelBase == NULL) {
        return false;
    }
    
    uint8_t *kernelBuffer;
    MALLOC(kernelBuffer, uint8_t*, KERNEL_READ_SIZE, M_TEMP, M_ZERO);
    
    if (kernelBuffer == NULL) {
        return false;
    }
    
    bool kernelFound = false;
    
    int paths_offset;
    
    for (paths_offset = 0; paths_offset < sizeof(kernel_paths) / sizeof(char*); paths_offset++) {
        if (readFile(kernel_paths[paths_offset], kernelBuffer, 0, KERNEL_READ_SIZE) == 0) {
            if (UUID_matches_running(kernelBuffer)) {
                kernelFound = true;
                break;
            }
        }
    }
    
    if (!kernelFound) {
        goto exit;
    }
    
    struct segment_command_64 *disk__TEXT = findSegmentLoadCommand(kernelBuffer, "__TEXT");
    if (disk__TEXT == NULL) {
        goto exit;
    }
    
    struct segment_command_64 *running__TEXT = findSegmentLoadCommand((uint8_t*) kInfo.kernelBase, "__TEXT");
    if (running__TEXT == NULL) {
        goto exit;
    }
    
    struct symtab_command *symtab = (struct symtab_command*) findLoadCommandOfType(kernelBuffer, NULL, LC_SYMTAB);
    if (symtab == NULL) {
        goto exit;
    }
    
    kInfo.diskTEXT        = disk__TEXT->vmaddr;
    kInfo.runningTEXT     = running__TEXT->vmaddr;
    kInfo.KASLR           = kInfo.runningTEXT - kInfo.diskTEXT;
    
    struct segment_command_64 *linkeditLoadCmd = findSegmentLoadCommand(kernelBuffer, "__LINKEDIT");
    if (linkeditLoadCmd == NULL) {
        goto exit;
    }
    
    uint8_t *linkedit;
    MALLOC(linkedit, uint8_t*, linkeditLoadCmd->filesize, M_TEMP, M_ZERO);
    if (linkedit == NULL) {
        goto exit;
    }
    
    if (readFile(kernel_paths[paths_offset], linkedit, linkeditLoadCmd->fileoff, linkeditLoadCmd->filesize) != 0) {
        FREE(linkedit, M_TEMP);
        goto exit;
    }
    
    kInfo.linkeditSegment  = linkedit;
    kInfo.linkeditSize     = linkeditLoadCmd->filesize;
    kInfo.symTable         = (uint8_t*) (linkedit + (symtab->symoff - linkeditLoadCmd->fileoff));
    kInfo.numOfSymbols     = symtab->nsyms;
    kInfo.stringTable      = (uint8_t*) (linkedit + (symtab->stroff - linkeditLoadCmd->fileoff));
    kInfo.stringTableSize  = symtab->strsize;
    kInfo.initializedMagic = MH_MAGIC;
    
    res = true;
    
exit:
    FREE(kernelBuffer, M_TEMP);
    return res;
}

void cleanupKernelInfo() {
    if (kInfo.initializedMagic != MH_MAGIC) {
        return;
    }
    
    kInfo.initializedMagic = MH_CIGAM;
    
    uint8_t *linkedit = kInfo.linkeditSegment;
    kInfo.linkeditSegment = NULL;
    kInfo.symTable = NULL;
    kInfo.stringTable = NULL;
    
    FREE(linkedit, M_TEMP);
}

void *findKernelSymbol(char *sym) {
    if (kInfo.initializedMagic != MH_MAGIC || kInfo.symTable == NULL || kInfo.stringTable == NULL || kInfo.numOfSymbols == 0 || kInfo.stringTableSize == 0) {
        return NULL;
    }
    
    struct nlist_64 *symEnts = (struct nlist_64*) kInfo.symTable;
    
    for (unsigned int i = 0; i < kInfo.numOfSymbols; i++) {
        char *symbolStr = (char*) ((mach_vm_address_t) kInfo.stringTable + symEnts[i].n_un.n_strx);
        if (strcmp(symbolStr, sym) == 0) {
            return (void*) ((mach_vm_address_t) symEnts[i].n_value + kInfo.KASLR);
        }
    }
    
    return NULL;
}

void enableKernelWrite() {
    asm volatile ("cli");
    
    uintptr_t cr0 = get_cr0();
    cr0 &= ~CR0_WP;
    set_cr0(cr0);
}

void disableKernelWrite() {
    uintptr_t cr0 = get_cr0();
    cr0 |= CR0_WP;
    set_cr0(cr0);
    
    asm volatile ("sti");
}
