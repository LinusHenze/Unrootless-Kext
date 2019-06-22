//
//  GlobalDefinitions.h
//  Unrootless
//
//  Created by Linus Henze on 12/10/2016.
//  Copyright © 2016 Linus Henze. All rights reserved.
//

#ifndef GlobalDefinitions_h
#define GlobalDefinitions_h

#define MAVERICKS   13
#define YOSEMITE    14
#define EL_CAPITAN  15
#define SIERRA      16
#define HIGH_SIERRA 17
#define MOJAVE      18
#define CATALINA    19

#if DEBUG
#define LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt "\n", ## __VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) ;
#endif

#define LOG_MSG(...) printf(__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ## __VA_ARGS__)
#define LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ## __VA_ARGS__)

#endif /* GlobalDefinitions_h */
