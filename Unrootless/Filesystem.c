//
//  Filesystem.c
//  Unrootless
//
//  Created by Linus Henze on 11/10/2016.
//  Copyright © 2016 Linus Henze. All rights reserved.
//

#include "Filesystem.h"

int readFile(char *file, uint8_t *buffer, off_t offset, user_size_t size) {
    int res = EIO;
    
    vfs_context_t vfsContext = vfs_context_create(NULL);
    if (vfsContext == NULL) {
        return EIO;
    }
    
    vnode_t fileVnode = NULLVP;
    if (vnode_lookup(file, 0, &fileVnode, vfsContext) == 0) {
        uio_t uio = uio_create(1, offset, UIO_SYSSPACE, UIO_READ);
        if (uio == NULL)
            goto exit;
        
        if (uio_addiov(uio, CAST_USER_ADDR_T(buffer), size))
            goto exit;
        
        if (VNOP_READ(fileVnode, uio, 0, vfsContext))
            goto exit;
        
        if (uio_resid(uio))
            goto exit;
        
        res = 0;
    } else {
        vfs_context_rele(vfsContext);
        return ENOENT;
    }
    
exit:
    vnode_put(fileVnode);
    vfs_context_rele(vfsContext);
    return res;
}
