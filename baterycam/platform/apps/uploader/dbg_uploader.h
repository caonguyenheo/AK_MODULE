#ifndef __dbg_uploader_h
#define __dbg_uploader_h

#include <stdio.h>

// #define NDEBUG  1
// // #define NDEBUG  1

// #ifndef NDEBUG
// #define debug(M, ...)
// #else
// //#define debug(M, ...) fprintf(stderr, "DEBUG %s (in function '%s'):%d:  " M "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
// #define debug(M, ...) printf("DEBUG %s (in function '%s'):%d:  " M "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
// #endif




#define jump_unless(A) if (!(A)) { goto error; }
#define error_unless(A, M, ...) if (!(A)) { fprintf(stderr, M "\n", ##__VA_ARGS__); goto error; }

#endif // __dbg_uploader_h
