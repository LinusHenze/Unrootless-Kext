//
//  Unrootless.c
//  Unrootless
//
//  Created by Linus Henze on 26.07.15.
//  Copyright © 2015 Linus Henze. All rights reserved.
//

#include <mach/mach_types.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <libkern/libkern.h>

#include "GlobalDefinitions.h"
#include "csr.h"
#include "Kernel.h"

extern const int version_major;

static uint32_t rootless_state = 0;
static uint32_t rootless_old_state = 0;
static uint32_t csr_flags = 0;
static uint32_t rootlessBootState = 0;
static int sysctl_rootless_disabled_func SYSCTL_HANDLER_ARGS;
static int sysctl_rootless_csrFlags SYSCTL_HANDLER_ARGS;

SYSCTL_NODE(_debug, // our parent
            OID_AUTO, // automatically assign us an object ID
            rootless, // our name
            CTLFLAG_RW, // we wil be creating children therefore, read/write
            0, // Handler function (none selected)
            "rootless"
);

SYSCTL_PROC(_debug_rootless, // our parent
            OID_AUTO, // automaticall assign us an object ID
            disabled, // our name
            (CTLTYPE_INT | // type flag
             CTLFLAG_RW  | CTLFLAG_ANYBODY), // access flag (read/write by anybody)
            &rootless_state, // location of our data
            0, // argument passed to our handler
            sysctl_rootless_disabled_func, // our handler function
            "IU", // our data type (unsigned integer)
            "enable/disable rootless" // our description
);

SYSCTL_PROC(_debug_rootless, // our parent
            OID_AUTO , // automaticall assign us an object ID
            csrConfig, // our name
            (CTLTYPE_INT | // type flag
             CTLFLAG_RW  | CTLFLAG_ANYBODY), // access flag (read/write by anybody)
            &csr_flags, // location of our data
            0, // argument passed to our handler
            sysctl_rootless_csrFlags, // our handler function
            "IU", // our data type (unsigned integer)
            "set csr flags" // our description
);

void setCSR(uint32_t flags) {
    enableKernelWrite();
    boot_args *args = (boot_args*) PE_state_loc->bootArgs;
    args->csrActiveConfig = flags;
    disableKernelWrite();
}

static int sysctl_rootless_disabled_func SYSCTL_HANDLER_ARGS {
    if (rootless_state == 2) { // Custom config
        // Check if we just set it to 2
        if (rootless_old_state != rootless_state) {
            sysctl_register_oid(&sysctl__debug_rootless_csrConfig);
            csr_flags = rootless_old_state ? ROOTLESS_DEFAULT_OFF : ROOTLESS_DEFAULT_ON;
            rootless_old_state = 2;
        }
        
        return sysctl_handle_int(oidp, oidp->oid_arg1, oidp->oid_arg2, req);
    }
    
    if (rootless_old_state == 2) {
        sysctl_unregister_oid(&sysctl__debug_rootless_csrConfig);
        boot_args *args = (boot_args*) PE_state_loc->bootArgs;
        args->flags &= ~(kBootArgsFlagCSRConfigMode);
    }
    
    rootless_state = (rootless_state > 0) ? 1 : 0;
    
    setCSR(rootless_state ? ROOTLESS_DEFAULT_OFF : ROOTLESS_DEFAULT_ON);
    
    rootless_old_state = rootless_state;
    
    return sysctl_handle_int(oidp, oidp->oid_arg1, oidp->oid_arg2, req);
}

static int sysctl_rootless_csrFlags SYSCTL_HANDLER_ARGS {
    csr_flags = csr_flags & CSR_VALID_FLAGS;
    boot_args *args = (boot_args*) PE_state_loc->bootArgs;
    if (csr_flags & CSR_ALLOW_DEVICE_CONFIGURATION) { // Allow csrutil enable/disable
        args->flags |= kBootArgsFlagCSRConfigMode;
    } else {
        args->flags &= ~(kBootArgsFlagCSRConfigMode);
    }
    
    setCSR(csr_flags);
    
    return sysctl_handle_int(oidp, oidp->oid_arg1, oidp->oid_arg2, req);
}

kern_return_t unrootless_start(kmod_info_t * ki, void *d) {
    if (version_major < EL_CAPITAN || version_major > CATALINA) {
        LOG_ERROR("You must run OS X >= El Capitan or macOS <= Catalina to unrootless.");
        return KERN_FAILURE;
    }
    
    if (!initKernelInfo()) {
        LOG_ERROR("initKernelInfo() failed!");
        return KERN_FAILURE;
    }
    
    PE_state_loc = (PE_state_t*) findKernelSymbol("_PE_state");
    if (PE_state_loc == NULL) {
        LOG_ERROR("Couldn't find '_PE_state', aborting...");
        return KERN_FAILURE;
    }
    
    boot_args *args = (boot_args*) PE_state_loc->bootArgs;
    rootlessBootState = args->csrActiveConfig;
    csr_flags = args->csrActiveConfig;
    if (!(args->flags & kBootArgsFlagCSRActiveConfig)) {
        LOG_INFO("Setting CSRActiveConfig flag...");
        args->flags |= kBootArgsFlagCSRActiveConfig; // Needed on some Macs...
    }
    
    rootless_state = !rootlessBootState;
    
    setCSR(rootless_state ? ROOTLESS_DEFAULT_OFF : ROOTLESS_DEFAULT_ON);
    
    sysctl_register_oid(&sysctl__debug_rootless);
    sysctl_register_oid(&sysctl__debug_rootless_disabled);
    
    return KERN_SUCCESS;
}

kern_return_t unrootless_stop(kmod_info_t *ki, void *d) {
    // Reset state
    setCSR(rootlessBootState);
    
    sysctl_unregister_oid(&sysctl__debug_rootless);
    sysctl_unregister_oid(&sysctl__debug_rootless_disabled);
    if (rootless_state == 2) {
        sysctl_unregister_oid(&sysctl__debug_rootless_csrConfig);
    }
    
    cleanupKernelInfo();
    
    return KERN_SUCCESS;
}
