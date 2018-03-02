/**
  ******************************************************************************
  * File Name          : debug_latency.h
  * Description        : This file contains all config debug_latency 
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
#ifndef __DEBUG_LATENCY_H__
#define __DEBUG_LATENCY_H__
#ifdef __cplusplus
 extern "C" {
#endif

/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <stdint.h> 

/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/

#define DBLTC_NOT_RUNNING          (0)
#define DBLTC_RUNNING              (1)

typedef struct db_latency
{
    unsigned long start_time;   // start time 
    unsigned long end_time;     // end time
    unsigned char running;      //DBLTC_NOT_RUNNING or DBLTC_RUNNING
    char          id;
}DB_LATENCY_t;
/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/
int dbltc_start(void);
void *dbltc_thread(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* __DEBUG_LATENCY_H__ */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/

