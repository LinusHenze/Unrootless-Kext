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

static u_int32_t rootless_state = 0;
static u_int32_t rootless_old_state = 0;
static u_int32_t csr_flags = 0;
static u_int32_t csr_orig_flags = 0;
static u_int32_t csr_orig_state = 0;
static int sysctl_rootless_disabled_func SYSCTL_HANDLER_ARGS;
static int sysctl_rootless_csrFlags SYSCTL_HANDLER_ARGS;

void (*_csr_set_allow_all)(int) = NULL;

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

kern_return_t setCSR(boolean_t flag) {
    enableKernelWrite();
    if (rootless_state == 2) {
        boot_args *args = (boot_args*) PE_state_loc->bootArgs;
        args->csrActiveConfig = csr_flags;
        disableKernelWrite();
        return KERN_SUCCESS;
    }
    if (_csr_set_allow_all != NULL) {
        if (flag == TRUE) {
            if (csr_orig_state == 1) { // Rootless was on on start
                _csr_set_allow_all(0);
            } else {
                if (PE_state_loc != NULL) {
                    boot_args *args = (boot_args*) PE_state_loc->bootArgs;
                    args->csrActiveConfig = ROOTLESS_DEFAULT_ON;
                }
            }
        } else {
            _csr_set_allow_all(1);
        }
    } else if (PE_state_loc != NULL) {
        boot_args *args = (boot_args*) PE_state_loc->bootArgs;
        if (flag == TRUE) {
            if (csr_orig_state == 1) { // Means rootless was on on start
                args->csrActiveConfig = csr_orig_flags;
            } else { // Means rootless was off on start
                args->csrActiveConfig = ROOTLESS_DEFAULT_ON;
            }
        } else {
            if (csr_orig_state == 1) { // Means rootless was on on start
                args->csrActiveConfig = ROOTLESS_DEFAULT_OFF;
            } else { // Means rootless was off on start
                args->csrActiveConfig = csr_orig_flags;
            }
        }
    }
    disableKernelWrite();
    return KERN_SUCCESS;
}

static int sysctl_rootless_disabled_func SYSCTL_HANDLER_ARGS {
    if (rootless_state == 2) { // Don't do anything
        if (rootless_old_state != rootless_state && PE_state_loc != NULL) {
            rootless_old_state = 2;
            sysctl_register_oid(&sysctl__debug_rootless_csrConfig);
            csr_flags = rootless_old_state ? ROOTLESS_DEFAULT_OFF : ROOTLESS_DEFAULT_ON;
        } else if (PE_state_loc == NULL) {
            rootless_state = rootless_old_state;
        }
        return sysctl_handle_int(oidp, oidp->oid_arg1, oidp->oid_arg2, req);
    }
    
    if (rootless_old_state == 2) {
        sysctl_unregister_oid(&sysctl__debug_rootless_csrConfig);
        boot_args *args = (boot_args*) PE_state_loc->bootArgs;
        args->flags &= ~(kBootArgsFlagCSRConfigMode);
    }
    
    rootless_state = (rootless_state > 0) ? 1 : 0;
    
    setCSR(!rootless_state);
    
    rootless_old_state = rootless_state;
    
    return sysctl_handle_int(oidp, oidp->oid_arg1, oidp->oid_arg2, req);
}

static int sysctl_rootless_csrFlags SYSCTL_HANDLER_ARGS {
    csr_flags = csr_flags & CSR_VALID_FLAGS;
    boot_args *args = (boot_args*) PE_state_loc->bootArgs;
    if (csr_flags & CSR_ALLOW_DEVICE_CONFIGURATION) { // Allow using csrutil enable/disable
        args->flags |= kBootArgsFlagCSRConfigMode;
    } else {
        args->flags &= ~(kBootArgsFlagCSRConfigMode);
    }
    setCSR(1); // Value doesn't care...
    
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
    
    _csr_set_allow_all = findKernelSymbol("_csr_set_allow_all");
    if (_csr_set_allow_all == NULL) {
        LOG_ERROR("Couldn't find '_csr_set_allow_all'");
        LOG_INFO("Trying _PE_state instead...");
    }
    
    PE_state_loc = (PE_state_t*) (findKernelSymbol("_PE_state"));
    if (PE_state_loc == NULL) {
        if (_csr_set_allow_all == NULL) {
            LOG_ERROR("Couldn't find '_PE_state' or '_csr_set_allow_all', aborting...");
            return KERN_FAILURE;
        }
    } else {
        boot_args *args = (boot_args*) PE_state_loc->bootArgs;
        csr_orig_flags = args->csrActiveConfig;
        csr_flags = csr_orig_flags;
        if (!(args->flags & kBootArgsFlagCSRActiveConfig)) {
            LOG_INFO("Setting CSRActiveConfig flag...");
            args->flags |= kBootArgsFlagCSRActiveConfig; // Needed on some mac's...
        }
    }
    
    int(*_csr_check)(int);
    _csr_check = findKernelSymbol("_csr_check");
    if (_csr_set_allow_all != NULL) {
        rootless_state = _csr_check(0);
        csr_orig_state = _csr_check(0);
    } else {
        rootless_state = !csr_orig_flags;
        csr_orig_state = !csr_orig_flags;
    }
    
    setCSR(!rootless_state);
    
    sysctl_register_oid(&sysctl__debug_rootless);
    sysctl_register_oid(&sysctl__debug_rootless_disabled);
    
    return KERN_SUCCESS;
}

kern_return_t unrootless_stop(kmod_info_t *ki, void *d) {
    setCSR(csr_orig_state);
    sysctl_unregister_oid(&sysctl__debug_rootless);
    sysctl_unregister_oid(&sysctl__debug_rootless_disabled);
    if (rootless_state == 2) {
        sysctl_unregister_oid(&sysctl__debug_rootless_csrConfig);
    }
    cleanupKernelInfo();
    return KERN_SUCCESS;
}
