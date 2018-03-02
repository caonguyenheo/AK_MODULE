#include "file_on_mem.h"
#include <stdio.h>
#include <string.h>
#include "ak_apps_config.h"
#include "libc_posix_fs.h"
//char my_file_buffer[MAX_FILEONMEM_LEN];
char *my_file_buffer=NULL;

FILE * f_mem_open(const char *filename, const char *mode){
#if (SDCARD_ENABLE==0)
	if (my_file_buffer==NULL)
		my_file_buffer = (char *)malloc(MAX_FILEONMEM_LEN*sizeof(char));

	struct fs_on_mem_struct* l_fs_on_mem=(struct fs_on_mem_struct*)my_file_buffer;
	if (l_fs_on_mem->signature==FILE_ON_MEM_SIGNATURE){
		if (strlen(l_fs_on_mem->filename)>0){
			if (mode[0]=='w')
				return (FILE*)NULL;
			else if (mode[0]=='r'){
				if (l_fs_on_mem->max_length==(MAX_FILEONMEM_LEN-sizeof(struct fs_on_mem_struct)-MAX_FILENAMEONMEM_LEN)){
					l_fs_on_mem->max_length=l_fs_on_mem->current_len;
					l_fs_on_mem->current_len=0;
				}
				//printf("l_fs_on_mem->max_length%d, l_fs_on_mem->current_len%d\n", l_fs_on_mem->max_length, l_fs_on_mem->current_len);
				return (FILE*)l_fs_on_mem; 
			} else
				return NULL;
		}
	}
	l_fs_on_mem->signature=FILE_ON_MEM_SIGNATURE;
	l_fs_on_mem->filename=my_file_buffer+sizeof(struct fs_on_mem_struct); //Filename string place after the struct
	memset(l_fs_on_mem->filename,0x00,MAX_FILENAMEONMEM_LEN);
	memcpy(l_fs_on_mem->filename,filename,strlen(filename));
	l_fs_on_mem->max_length=MAX_FILEONMEM_LEN-sizeof(struct fs_on_mem_struct)-MAX_FILENAMEONMEM_LEN;
	l_fs_on_mem->filebin=my_file_buffer+sizeof(struct fs_on_mem_struct)+MAX_FILENAMEONMEM_LEN;
	l_fs_on_mem->current_len=0;
	return (FILE*)l_fs_on_mem;
#else
	return fopen(filename, mode);
#endif
}

size_t f_mem_write(const void *ptr, size_t size, size_t nmemb, FILE *stream){
#if (SDCARD_ENABLE==0)
	struct fs_on_mem_struct* l_fs_on_mem=(struct fs_on_mem_struct*)stream;
	char *src_ptr = (char*) ptr;
	char *dest_ptr = l_fs_on_mem->filebin+l_fs_on_mem->current_len;
	int l_len = size * nmemb;
	if ((src_ptr!=NULL) && (dest_ptr!=NULL)){
		if ((l_fs_on_mem->current_len+l_len)<=l_fs_on_mem->max_length){
			memcpy(dest_ptr,src_ptr,l_len);
			l_fs_on_mem->current_len+=l_len;
			return nmemb;
		}else
			return FILEONMEM_ERROR_OUTOFBUF;
	}else
		return FILEONMEM_ERROR_MEMORYNULL;
#else
	return fwrite(ptr,size,nmemb,stream);
#endif
}

size_t f_mem_read(void *ptr, size_t size, size_t nmemb, FILE *stream){
#if (SDCARD_ENABLE==0)
	struct fs_on_mem_struct* l_fs_on_mem=(struct fs_on_mem_struct*)stream;
	char *dest_ptr = (char*) ptr;
	char *src_ptr = l_fs_on_mem->filebin+l_fs_on_mem->current_len;
	int l_len = size * nmemb;
	int temp=0;
	//printf("l_fs_on_mem->max_length%d, l_fs_on_mem->current_len%d\n", l_fs_on_mem->max_length, l_fs_on_mem->current_len);
	if ((src_ptr!=NULL) && (dest_ptr!=NULL)){
		if ((l_fs_on_mem->current_len+l_len)<=l_fs_on_mem->max_length)
			temp = l_len;
		else
			temp = l_fs_on_mem->max_length-l_fs_on_mem->current_len;
			
		memcpy(dest_ptr,src_ptr,temp);
		l_fs_on_mem->current_len+=temp;
		//printf("f_mem_read%d, temp%d\n", l_fs_on_mem->current_len, temp);
		return (size_t)(temp/size);
	}else
		return FILEONMEM_ERROR_MEMORYNULL;
#else
	return fread(ptr, size, nmemb, stream);
#endif
}

int f_mem_close(FILE *stream){
#if SDCARD_ENABLE==0
	return 0;
#else
	return fclose(stream);
#endif
}
#if (SDCARD_ENABLE==0)
int f_mem_erase(char *filename){
	struct fs_on_mem_struct* l_fs_on_mem=(struct fs_on_mem_struct*)my_file_buffer;
	if (l_fs_on_mem->signature==FILE_ON_MEM_SIGNATURE)
		l_fs_on_mem->filename[0]=0;
	return 0;
}
#endif
int f_mem_flush(FILE *stream){
#if (SDCARD_ENABLE==0)
	return 0;
#else
	return fflush(stream);
#endif
}
