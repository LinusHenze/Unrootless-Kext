//
//  Kernel.h
//  Unrootless
//
//  Created by Linus Henze on 11/10/2016.
//  Copyright © 2016 Linus Henze. All rights reserved.
//

#ifndef Kernel_h
#define Kernel_h

#include <mach/kern_return.h>
#include <sys/types.h>
#include <mach/mach_vm.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <sys/systm.h>
#include <i386/proc_reg.h>

extern struct KernelInfo kInfo;

struct __attribute__((packed)) IDT_Desc_AMD64 {
    uint16_t offset_low;    // bits 0...15
    uint16_t selector;
    uint8_t  int_stck_tbl;
    uint8_t  type_attr;
    uint16_t offset_middle; // bits 16...31
    uint32_t offset_high;   // bits 32...63
    uint32_t reserved;
};

struct KernelInfo {
    uint8_t           *kernelBase;
    mach_vm_address_t runningTEXT;
    mach_vm_address_t diskTEXT;
    mach_vm_address_t KASLR;
    uint8_t           *symTable;
    uint32_t          numOfSymbols;
    uint8_t           *stringTable;
    uint32_t          stringTableSize;
    uint8_t           *linkeditSegment;
    uint64_t          linkeditSize;
    uint32_t          initializedMagic;
};

extern struct KernelInfo kInfo;

mach_vm_address_t findKernelBase(void);
bool initKernelInfo(void);
void cleanupKernelInfo(void);
void *findKernelSymbol(char *sym);

void enableKernelWrite(void);
void disableKernelWrite(void);

#endif /* Kernel_h */
