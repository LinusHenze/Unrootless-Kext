//
//  csr.h
//  Unrootless
//
//  Created by Linus Henze on 13/03/16.
//  Copyright © 2016 Linus Henze. All rights reserved.
//

#ifndef csr_h
#define csr_h

struct PE_Video {
    unsigned long   v_baseAddr;     /* Base address of video memory */
    unsigned long   v_rowBytes;     /* Number of bytes per pixel row */
    unsigned long   v_width;        /* Width */
    unsigned long   v_height;       /* Height */
    unsigned long   v_depth;        /* Pixel Depth */
    unsigned long   v_display;      /* Text or Graphics */
    char		v_pixelFormat[64];
    long		v_resv[ 4 ];
};

typedef struct PE_Video       PE_Video;

typedef struct PE_state {
    boolean_t	initialized;
    PE_Video	video;
    void		*deviceTreeHead;
    void		*bootArgs;
#if __i386__
    void		*fakePPCBootArgs;
#endif
} PE_state_t;

#define kBootArgsFlagCSRConfigMode	(1 << 4)

PE_state_t *PE_state_loc = NULL;

#define BOOT_LINE_LENGTH        1024

struct Boot_Video {
    uint32_t	v_baseAddr;	/* Base address of video memory */
    uint32_t	v_display;	/* Display Code (if Applicable */
    uint32_t	v_rowBytes;	/* Number of bytes per pixel row */
    uint32_t	v_width;	/* Width */
    uint32_t	v_height;	/* Height */
    uint32_t	v_depth;	/* Pixel Depth */
};

typedef struct Boot_Video	Boot_Video;

typedef struct boot_args {
    uint16_t    Revision;	/* Revision of boot_args structure */
    uint16_t    Version;	/* Version of boot_args structure */
    
    uint8_t     efiMode;    /* 32 = 32-bit, 64 = 64-bit */
    uint8_t     debugMode;  /* Bit field with behavior changes */
    uint16_t    flags;
    
    char        CommandLine[BOOT_LINE_LENGTH];	/* Passed in command line */
    
    uint32_t    MemoryMap;  /* Physical address of memory map */
    uint32_t    MemoryMapSize;
    uint32_t    MemoryMapDescriptorSize;
    uint32_t    MemoryMapDescriptorVersion;
    
    Boot_Video	Video;		/* Video Information */
    
    uint32_t    deviceTreeP;	  /* Physical address of flattened device tree */
    uint32_t    deviceTreeLength; /* Length of flattened tree */
    
    uint32_t    kaddr;            /* Physical address of beginning of kernel text */
    uint32_t    ksize;            /* Size of combined kernel text+data+efi */
    
    uint32_t    efiRuntimeServicesPageStart; /* physical address of defragmented runtime pages */
    uint32_t    efiRuntimeServicesPageCount;
    uint64_t    efiRuntimeServicesVirtualPageStart; /* virtual address of defragmented runtime pages */
    
    uint32_t    efiSystemTable;   /* physical address of system table in runtime area */
    uint32_t    kslide;
    
    uint32_t    performanceDataStart; /* physical address of log */
    uint32_t    performanceDataSize;
    
    uint32_t    keyStoreDataStart; /* physical address of key store data */
    uint32_t    keyStoreDataSize;
    uint64_t	bootMemStart;
    uint64_t	bootMemSize;
    uint64_t    PhysicalMemorySize;
    uint64_t    FSBFrequency;
    uint64_t    pciConfigSpaceBaseAddress;
    uint32_t    pciConfigSpaceStartBusNumber;
    uint32_t    pciConfigSpaceEndBusNumber;
    uint32_t	csrActiveConfig;
    uint32_t	csrPendingConfig;
    uint32_t    __reserved4[728];
    
} boot_args;

#define CSR_ALLOW_UNTRUSTED_KEXTS		(1 << 0)
#define CSR_ALLOW_UNRESTRICTED_FS		(1 << 1)
#define CSR_ALLOW_TASK_FOR_PID			(1 << 2)
#define CSR_ALLOW_KERNEL_DEBUGGER		(1 << 3)
#define CSR_ALLOW_APPLE_INTERNAL		(1 << 4)
#define CSR_ALLOW_DESTRUCTIVE_DTRACE	(1 << 5) /* name deprecated */
#define CSR_ALLOW_UNRESTRICTED_DTRACE	(1 << 5)
#define CSR_ALLOW_UNRESTRICTED_NVRAM	(1 << 6)
#define CSR_ALLOW_DEVICE_CONFIGURATION  (1 << 7)

#define CSR_VALID_FLAGS (CSR_ALLOW_UNTRUSTED_KEXTS | \
CSR_ALLOW_UNRESTRICTED_FS | \
CSR_ALLOW_TASK_FOR_PID | \
CSR_ALLOW_KERNEL_DEBUGGER | \
CSR_ALLOW_APPLE_INTERNAL | \
CSR_ALLOW_UNRESTRICTED_DTRACE | \
CSR_ALLOW_UNRESTRICTED_NVRAM | \
CSR_ALLOW_DEVICE_CONFIGURATION)

#define ROOTLESS_DEFAULT_ON  0
#define ROOTLESS_DEFAULT_OFF 103

#endif /* csr_h */
