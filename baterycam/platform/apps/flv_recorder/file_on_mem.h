#ifndef _FILE_ON_MEM_H_
#define _FILE_ON_MEM_H_
#include <stdio.h>

/*File on mem structure
    - File INFO struction
        1 int file signature 0x012345AF
        1 char* to filename
        1 int max length
        1 char* to file binary
        1 int current file length
    - Filename 64 bytes
    - File binary
*/
struct fs_on_mem_struct {
    int signature;
    char* filename;
    int max_length;
    char* filebin;
    int current_len;
};

#define MAX_FILEONMEM_LEN (2*1024*1024)    
#define MAX_FILENAMEONMEM_LEN 64
#define FILE_ON_MEM_SIGNATURE (0x012345AF)
#define FILE_ON_MEM_NOFILESIGNATURE (0)

FILE * f_mem_open(const char *filename, const char *mode);
size_t f_mem_write(const void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t f_mem_read(void  *ptr, size_t size, size_t nmemb, FILE *stream);
	
enum file_on_mem_code {
    FILEONMEM_OK = 0,
    FILEONMEM_ERROR_MEMORYNULL = -1,
    FILEONMEM_ERROR_OUTOFBUF = -2,
    FILEONMEM_ERROR_FILEEXIST = -3,
};
#endif