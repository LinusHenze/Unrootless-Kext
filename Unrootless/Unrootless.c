//
//  Test.c
//  Test
//
//  Created by Linus Henze on 26.07.15.
//  Copyright © 2015 Linus Henze. All rights reserved.
//

#include <mach/mach_types.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include "my_data_definitions.h"
#include "sysent.h"
#include "syscall.h"
#include "kernel_info.h"
#include "cpu_protections.h"

kern_return_t Test_start(kmod_info_t * ki, void *d);
kern_return_t Test_stop(kmod_info_t *ki, void *d);

struct kernel_info g_kernel_info;
extern const int version_major;

static u_int32_t k_uint32 = 0;
static int sysctl_rootless_uint32 SYSCTL_HANDLER_ARGS;

void (*_csr_set_allow_all)(int);

SYSCTL_NODE(_debug, // our parent
            OID_AUTO , // automatically assign us an object ID
            rootless , // our name
            CTLFLAG_RW, // we wil be creating children therefore , read/write
            0, // Handler function ( none selected )
            "rootless"
);

SYSCTL_PROC ( _debug_rootless, //our parent
             OID_AUTO , // automaticall assign us an object ID
             disabled, // our name
             ( CTLTYPE_INT | // type flag
              CTLFLAG_RW | CTLFLAG_ANYBODY ), //access flag ( read/write by anybody )
             &k_uint32, // location of our data
             0, //argument passed to our handler
             sysctl_rootless_uint32, //our handler function
             "IU", // our data type ( unsigned integer )
             "enable/disable rootless" // our description
);

static int sysctl_rootless_uint32 SYSCTL_HANDLER_ARGS {
    
    k_uint32 = (k_uint32 > 0) ? 1 : 0;
    enable_kernel_write();
    _csr_set_allow_all(k_uint32);
    disable_kernel_write();
    
    return sysctl_handle_int( oidp, oidp->oid_arg1 , oidp->oid_arg2 , req );
}

kern_return_t unrootless_start(kmod_info_t * ki, void *d)
{
    if (version_major != EL_CAPITAN) {
        LOG_ERROR("You must run OS X El Capitan to unrootless.");
        return KERN_FAILURE;
    }
    /* locate sysent table */
    mach_vm_address_t kernel_base = 0;
    if (find_sysent(&kernel_base) != KERN_SUCCESS)
    {
        return KERN_FAILURE;
    }
    /* read kernel info from the disk image */
    if (init_kernel_info(&g_kernel_info, kernel_base) != KERN_SUCCESS)
    {
        return KERN_FAILURE;
    }
    _csr_set_allow_all=(void*)solve_kernel_symbol(&g_kernel_info, "_csr_set_allow_all");
    if (_csr_set_allow_all == NULL) {
        LOG_ERROR("Couldn't find '_csr_set_allow_all'");
        return KERN_FAILURE;
    }
    int(*_csr_check)(int);
    _csr_check = (void*)solve_kernel_symbol(&g_kernel_info, "_csr_check");
    _csr_set_allow_all(1);
    k_uint32 = !_csr_check(0);
    
    sysctl_register_oid(&sysctl__debug_rootless);
    sysctl_register_oid(&sysctl__debug_rootless_disabled);
    
    return KERN_SUCCESS;
}

kern_return_t unrootless_stop(kmod_info_t *ki, void *d)
{
    cleanup_sysent();
    _csr_set_allow_all(0);
    sysctl_unregister_oid(&sysctl__debug_rootless);
    sysctl_unregister_oid(&sysctl__debug_rootless_disabled);
    return KERN_SUCCESS;
}
