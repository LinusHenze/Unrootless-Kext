//
//  Filesystem.h
//  Unrootless
//
//  Created by Linus Henze on 11/10/2016.
//  Copyright © 2016 Linus Henze. All rights reserved.
//

#ifndef Filesystem_h
#define Filesystem_h

#include <sys/vnode.h>

int readFile(char *file, uint8_t *buffer, off_t offset, user_size_t size);

#endif /* Filesystem_h */
