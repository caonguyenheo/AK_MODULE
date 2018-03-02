/**
  ******************************************************************************
  * File Name          : apps_log.h
  * Description        :  
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 NXCOMM PTE LTD
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of NXCOMM PTE LTD nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APPS_LOG_H__
#define __APPS_LOG_H__
#ifdef __cplusplus
 extern "C" {
#endif

/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <stdio.h>
 

/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/
//define color
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"


#define NDEBUG  1

#ifndef NDEBUG
#define apps_info(M, ...)
#else
#define apps_info(M, ...) fprintf(stderr, "INFO (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#ifndef NDEBUG
#define apps_debug(M, ...)
#else
#define apps_debug(M, ...) fprintf(stderr, ANSI_COLOR_BLUE "DEBUG (%s:%d) " M "\n" ANSI_COLOR_RESET, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#ifndef NDEBUG
#define apps_warn(M, ...)
#else
#define apps_warn(M, ...) fprintf(stderr, ANSI_COLOR_YELLOW "WARN (%s:%d) " M "\n" ANSI_COLOR_RESET, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#ifndef NDEBUG
#define apps_error(M, ...)
#else
#define apps_error(M, ...) fprintf(stderr, ANSI_COLOR_RED "ERROR (%s:%d) " M "\n" ANSI_COLOR_RESET, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

//#define jump_unless(A) if (!(A)) { goto error; }
//#define error_unless(A, M, ...) if (!(A)) { fprintf(stderr, M "\n", ##__VA_ARGS__); goto error; }


#define dzlog_info                  printf
#define dzlog_debug                 printf
#define dzlog_warn                  printf
#define dzlog_error                 printf
/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/


#ifdef __cplusplus
}
#endif

#endif /* __GM_LOG_H__ */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/
