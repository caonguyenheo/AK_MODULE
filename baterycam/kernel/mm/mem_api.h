/************************************************************************************
* Copyright(c) 2006 Anyka.com
* All rights reserved.
*
* File	:	mem_api.h
* Brief :	heap memory allocator
*           
* 
* Version : 4.0.9
* Author  : ZhangMuJun
* Modify  : ZhangMuJun
* Data    : 2008-12-18
*************************************************************************************/
#ifndef __MEM_API_H__
#define __MEM_API_H__


#ifdef __cplusplus
extern "C" {
#endif


#include "anyka_types.h"

#define  MEMLIB_VERSION		("5.0.00")


/**
 * @brief  Global heap memory allocator handler
 */
typedef void *	T_GLOBALMEMHandle;
typedef long 	(*Ram_PrintfTraceFun)(const char *format, ...); 

/**
 * @brief  Thread/Reentry Control Function
 * @brief  zero will single thread ; else will multithread
 *
 * @note   Using Ram_SetLock must after  Ram_Initial(Ex).
 *         Using Ram_ClrLock must before Ram_Exit.
 *         Platform User Porting Ram_Lock/Ram_Unlock
 */
extern  void	Ram_Lock(unsigned long dwLock);
extern  void	Ram_Unlock(unsigned long dwLock);

void	Ram_ClrLock(T_GLOBALMEMHandle hMemory);
void	Ram_SetLock(T_GLOBALMEMHandle hMemory, unsigned long dwLock);

/* ********************************* ȫ���ڴ�ѷ����� ********************************* */

/**
 * @brief  Initialize Global heap memory allocator
 * @brief  ���ڴ�����ʵ����
 *
 * @author  ZMJ
 * @date	6/24/2006 
 *
 * @param	unsigned char  *ptr :	global heap memory start adress
 * @param	unsigned long size :	global heap memory length           (����С�� 2^(26+align2) , align2��align��2�ݴ�)
 * @param	unsigned short llenpad : ǰ�߽����ó��� (���ڼ���ڴ�ǰԽ��) (Ĭ��Ϊ0 0:����� ����ֵ:���)
 * @param   unsigned char  padchk :  �������ܼ��						(Ĭ��Ϊ0 0:������)
 * @param	unsigned short rlenpad : ��߽����ó��� (���ڼ���ڴ��Խ��) (Ĭ��Ϊ0 0:����� ����ֵ:���)
 * @param	unsigned char  lvar :	ǰ�߽�Ԥ���ֵ (���ڼ���ڴ�ǰԽ��)	(Ĭ��Ϊ0xFC)
 * @param	unsigned char  rvar :	��߽�Ԥ���ֵ (���ڼ���ڴ��Խ��)	(Ĭ��Ϊ0xCF)
 * @param	unsigned char align :	�ڴ�������   (2^[3,4,5,6,7, 8])	(Ĭ��Ϊ16)
 * @param	unsigned long lenfree : �ͷ����еĳ��� (32X)				(Ĭ��Ϊ6400) [����С�� 2^14; ���ڴ�=lenfree*align]
 * @param	unsigned char  sos :		�����ڴ���ϵĿ�����ռ���ڴ����  (Ĭ��Ϊ50) [��������Ӧ���������ٶȺ���Ƭ���ö� : ��ǿ������]
 * @param   unsigned char split :    ���ݲ������ѵ���С����			(Ĭ��Ϊ4) [���ڼ���С��Ƭ����Ŀ]
 * @param   unsigned char  wildchk : ����Ұ�������						(Ĭ��Ϊ0  0:����� 0x10:ֻ����ڴ�ͷ8���ֽ� 0x01:ֻ����ڴ�ͷ8���ֽ��� 0x11:ȫ��� )  
 * @param   unsigned char  wvar :    δʹ���ڴ�Ԥ���ֵ(���ڼ��Ұָ��)  (Ĭ��Ϊ0xCC) 
 * @param   Ram_PrintfTraceFun lpPrintf : �Ƿ��ӡ��Ϣ			(Ĭ��ΪAK_NULL, ����ӡ)
 *
 * @return  ram handle for success;else for AK_NULL
 *
 */
T_GLOBALMEMHandle	Ram_Initial(unsigned char *ptr, unsigned long size);

T_GLOBALMEMHandle	Ram_InitialEx(unsigned char  *ptr,     unsigned long size,											//�Ѳ���
								  unsigned char  padchk,   unsigned short llenpad,  unsigned short rlenpad, unsigned char lvar, unsigned char rvar,  //Խ�������
								  unsigned char  wildchk,  unsigned char wvar,											//Ұ����������
								  unsigned char  align,    unsigned long lenfree, unsigned char sos, unsigned char split,					//�߼��������Ż�����
								  Ram_PrintfTraceFun lpPrintf);


/**
 * @brief  Destroy Global heap memory allocator
 * @brief  ���ڴ�����ʵ����
 * @
 * @author  ZMJ
 * @date	6/24/2006 
 *
 * @param	T_GLOBALMEMHandle hMemory : memory allocator handler
 *
 * @return  AK_NULL
 *
 */
T_GLOBALMEMHandle	Ram_Exit(T_GLOBALMEMHandle hMemory);


/**
 * @brief  Malloc one memory block from global heap memory
 *
 * @author  ZMJ
 * @date	6/24/2006 
 *
 * @param	T_GLOBALMEMHandle hMemory : memory allocator handler
 * @param	unsigned long size : want memory size 
 * @param   char *filename : alloc hander location filename
 * @param   char *fileline : alloc hander location fileline
 *
 * @return  void * : memory address for success, AK_NULL for failure 
 *
 */
void *	Ram_Alloc(T_GLOBALMEMHandle hMemory, unsigned long size, signed char *filename, unsigned long fileline);


/**
 * @brief  Remalloc one memory block from global heap memory
 *
 * @author  ZMJ
 * @date	6/24/2006 
 *
 * @param	T_GLOBALMEMHandle hMemory : memory allocator handler
 * @param   void * var : old memory address
 * @param	unsigned long size : want memory size 
 * @param   char *filename : alloc hander location filename
 * @param   char *fileline : alloc hander location fileline
 *
 * @return  void * : new memory address for success, AK_NULL for failure 
 *
 */
void *	Ram_Realloc(T_GLOBALMEMHandle hMemory, void * var, unsigned long size, signed char *filename, unsigned long fileline); 

/**
 * @brief  Free one memory block to global heap memory
 *
 * @author  ZMJ
 * @date	6/24/2006 
 *
 * @param	T_GLOBALMEMHandle hMemory : memory allocator handler
 * @param	void * var : memory start address alloced before 
 * @param   char *filename : alloc hander location filename
 * @param   char *fileline : alloc hander location fileline
 *
 * @return  void * : AK_NULL
 *
 */
void * Ram_Free(T_GLOBALMEMHandle hMemory, void * var, signed char *filename, unsigned long fileline);


/**
 * @brief  Debug memory relation info
 * @
 * @memory block map
 */
typedef struct tagT_MEMORY_TRACE
{
	void *		addr;		//memory ponter
	long		size;		//memory size
	unsigned long		line;		//memory alloc location line
	char		*filename;	//memory alloc location file
	unsigned char		old;		//used or not
	long		reqSize;	//real request alloc size
}T_MEMORY_TRACE, *T_lpMEMORY_TRACE;

/**
 * @brief  Debug memory relation info
 * @
 * @memory map callback func define 
 */
typedef void	(*Ram_EnumMemTraceFun)(T_GLOBALMEMHandle hMemory, const T_MEMORY_TRACE *map, void * attatch);



/**
 * @brief  Enumerate memory statck status info
 * @ֻ�����ڲ�ѯ�ڴ�״̬�ͼ����ʽ�ڴ�й©���
 *
 * @author  ZMJ
 * @date	6/24/2006 
 *
 * @param	T_GLOBALMEMHandle hMemory : memory allocator handler
 * @param	Ram_EnumMemTraceFun enumFun : user define memory map callback query and prontf function
 * @param   void * attach : user self data needed by callback function
 *
 * @return  void * : AK_NULL
 *
 */
void Ram_EnumMemTraceInfo(T_GLOBALMEMHandle hMemory, Ram_EnumMemTraceFun enumFun, void * attach);


/**
 * @brief  Debug memory heat info
 * @
 * @memory block map
 */
typedef struct tagT_MEMORY_STATUS
{
	unsigned long		line;		//memory alloc location line
	char		*filename;	//memory alloc location file
	unsigned long		hits;		//location handler alloc times : free will --
	unsigned long		ratio;		//location handler ratio of all (��Handler����������ڴ�/�����Ѿ���������ڴ�)*100
}T_MEMORY_STATUS, *T_lpMEMORY_STATUS;

/**
 * @brief  Debug memory heat info
 * @
 * @memory map callback func define 
 */
typedef void	(*Ram_EnumMemStatusFun)(T_GLOBALMEMHandle hMemory, const T_MEMORY_STATUS *map, void * attatch);

/**
 * @brief  Enumerate memory statck status info
 * @ֻ�����ڲ�ѯ�ڴ��ȵ�״̬�ͼ����ʽ�ڴ�й©���
 * @����Ϊ��һ���Ż��ṩ����
 *
 * @author  ZMJ
 * @date	6/24/2006 
 *
 * @param	T_GLOBALMEMHandle hMemory : memory allocator handler
 * @param	Ram_EnumMemStatusFun enumFun : user define memory map callback query and prontf function
 * @param   void * attach : user self data needed by callback function
 *
 * @return  void * : AK_NULL
 *
 */
void Ram_EnumMemStatusInfo(T_GLOBALMEMHandle hMemory, Ram_EnumMemStatusFun enumFun, void * attach);



/**
 * @brief  Debug memory wild info
 * @
 * @memory block map
 */
typedef enum tagT_WILD_TYPE
{
	WILD_OK			= 0,		//ptr����Чָ��
	WILD_WILD		= 0x100,	//ptr��Ұָ��
	WILD_WILD_PREV	= 0x101,	//ptr��Ұָ��, �������Ƿ�дǰ8���ֽڶ���
	WILD_WILD_BACK	= 0x110,	//ptr��Ұָ��, �������Ƿ�д8���ֽں����ݶ���
	WILD_WILD_BOTH	= 0x111,	//ptr��Ұָ��, �������Ƿ�д����8���ֽ����ݶ���
	WILD_INVALID    = 0x200,	//ptr����Чָ��(����ĳ����Ч�ڴ�飬���ڴ�ͷ�����ڴ�ͷ���ٻ�)
	WILD_INVALID_UNHEAP			= 0x210,	//ptr����Чָ��(���ڶ��ڴ��ַ��Χ��)
	WILD_INVALID_INHEAP			= 0x220,	//ptr����Чָ��(�Ѿ����ո����ڴ�)
	WILD_INVALID_INHEAP_WILD	= 0x221,	//ptr����Чָ��(�Ѿ����ո����ڴ�),�������Ƿ�д���ݶ���
	WILD_ERR					//UnKnwon error
}T_WILD_TYPE;
    
typedef struct tagT_MEMORY_WILDER_TRACE
{
	void *		addr;		//memory ponter
	long		size;		//memory size
	unsigned long		line;		//memory alloc location line
	char		*filename;	//memory alloc location file
	long		reqSize;	//real request alloc size
	T_WILD_TYPE type;		//wild type
}T_MEMORY_WILDER_TRACE, *T_lpMEMORY_WILDER_TRACE;


/**
 * @brief  Enumerate memory statck status info
 * @ֻ�����ڼ��Ұ�������
 * @�Ӷ��ڴ�ͷ��ʼ���,��⵽��һ��Ұ����ʹ�ü��˳�
 * @author  ZMJ
 * @date	6/24/2006 
 *
 * @param	T_GLOBALMEMHandle hMemory : memory allocator handler
 * @param	T_MEMORY_WILDER_TRACE *wildInfo : wild info loader
 *
 * @return  AK_TRUE for no wild ;else for AK_FALSE
 *
 */
bool Ram_CheckWilder(T_GLOBALMEMHandle hMemory, T_MEMORY_WILDER_TRACE *wildInfo);   

/**
 * @brief  Enumerate memory statck status info
 * @ֻ������ֱ�Ӽ��ָ��ָ���Ұ�������
 * @author  ZMJ
 * @date	6/24/2006 
 *
 * @param	T_GLOBALMEMHandle hMemory : memory allocator handler
 * @param	T_WILD_TYPE *wildType : wild type
 *
 * @return  AK_TRUE for no beyond ;else for AK_FALSE
 *
 */
bool Ram_CheckWildrPtr(T_GLOBALMEMHandle hMemory, void *ptr, T_WILD_TYPE *wildType);


/**
 * @brief  Debug memory beyond info
 * @
 * @memory block map
 */
typedef enum tagT_BEYOND_TYPE
{
	BEYOND_OK		= 0x000,	//ptr��Խ��
	BEYOND_PREV		= 0x101,	//ptr������ǰ��Խ��
	BEYOND_BACK		= 0x110,	//ptr����������Խ��
	BEYOND_BOTH		= 0x111,	//ptr������ȫԽ��
	BEYOND_INVALID	= 0x2000,	//ptr����Чָ��
	BEYOND_INVALID_WILD		= BEYOND_INVALID+WILD_WILD,				//ptr��Ұָ��,�����
	BEYOND_INVALID_INVALID	= BEYOND_INVALID+WILD_INVALID,			//ptr����Чָ��,�����
	BEYOND_INVALID_UNHEAP	= BEYOND_INVALID+WILD_INVALID_UNHEAP,	//ptr����Чָ��(���ڶ��ڴ��ַ��Χ��), �����
	BEYOND_INVALID_INHEAP	= BEYOND_INVALID+WILD_INVALID_INHEAP,	//ptr����Чָ��(�Ѿ����ո����ڴ�), �����
	BEYOND_ERR					//UnKnwon error
}T_BEYOND_TYPE;

typedef struct tagT_MEMORY_BEYOND_TRACE
{
	void *		addr;		//memory ponter
	long		size;		//memory size
	unsigned long		line;		//memory alloc location line
	char		*filename;	//memory alloc location file
	unsigned char		old;		//used or not
	long		reqSize;	//real request alloc size
	unsigned short		loc;		//first beyond location <only useful for BEYOND_BACK>
	unsigned short		cnt;		//beyond amount
	T_BEYOND_TYPE type;		//beyond type
}T_MEMORY_BEYOND_TRACE, *T_lpMEMORY_BEYOND_TRACE;

/**
 * @brief  Enumerate memory statck status info
 * @ֻ�����ڼ���ڴ�Խ�����
 * @�Ӷ��ڴ�ͷ��ʼ���,��⵽��һ��Խ�缴�˳�
 * @author  ZMJ
 * @date	6/24/2006 
 *
 * @param	T_GLOBALMEMHandle hMemory : memory allocator handler
 * @param	T_MEMORY_BEYOND_TRACE *beyondInfo : beyond info loader
 *
 * @return  AK_TRUE for no beyond ;else for AK_FALSE
 *
 */
bool Ram_CheckBeyond(T_GLOBALMEMHandle hMemory, T_MEMORY_BEYOND_TRACE *beyondInfo);   

/**
 * @brief  Enumerate memory statck status info
 * @ֻ������ֱ�Ӽ��ָ���ڴ�Խ�����
 * @author  ZMJ
 * @date	6/24/2006 
 *
 * @param	T_GLOBALMEMHandle hMemory : memory allocator handler
 * @param	T_BEYOND_TYPE *beyondType : beyond type
 *
 * @return  AK_TRUE for no beyond ;else for AK_FALSE
 *
 */
bool Ram_CheckBeyondPtr(T_GLOBALMEMHandle hMemory, void *ptr, T_BEYOND_TYPE *beyondType);


/**
 * @brief  �������Զ����ڴ�й©�����
 * @
 * @memory auto callback func define 
 */
typedef void	(*Ram_EnterStateMachine)(T_GLOBALMEMHandle hMemory);
typedef void	(*Ram_LeaveStateMachine)(T_GLOBALMEMHandle hMemory);

/**
 * @brief  Ҫ������ε��ڴ�й©״�� : ��ʼλ��
 * @�뱣�����ʹ��
 *
 * @param	T_GLOBALMEMHandle hMemory : memory allocator handler
 */
void  RAM_ENTER_LEAKCHK_SECTION(T_GLOBALMEMHandle hMemory);


/**
 * @brief  Ҫ������ε��ڴ�й©״�� : ����λ��
 * @�뱣�����ʹ��
 *
 * @param	T_GLOBALMEMHandle hMemory : memory allocator handler
 */
void	RAM_LEAVE_LEAKCHK_SECTION(T_GLOBALMEMHandle hMemory);



/**
 * @brief  enable memory leak auto trace mechnics
 * @ֻ������״̬�����ڴ�й©�Զ���� 
 * @��2000�ڴ�������,��20��״̬��ջ�������
 * @author  ZMJ
 * @date	6/24/2006 
 *
 * @param	T_GLOBALMEMHandle hMemory : memory allocator handler
 * @param	Ram_EnumMemTraceFun enumFun : callback printf func
 * @param   unsigned long logCnt : log memory count <default is 2000>
 * @param   Ram_EnterStateMachine *enterSection : GET HOOK for enter critical leakchk callback function
 * @param   Ram_LeaveStateMachine *leaveSection : GET HOOK for leave critical leakchk callback function
 *
 * @return  void
 *
 * @return  AK_TRUE for auto ok else return false
 *
 */
bool Ram_EnableAutoLeakTrace(T_GLOBALMEMHandle hMemory, Ram_EnumMemTraceFun enumFun, unsigned long logCnt, Ram_EnterStateMachine *enterSection, Ram_LeaveStateMachine *leaveSection);

/**
 * @brief  disable memory leak auto trace mechnics
 * @ֻ������״̬�����ڴ�й©�Զ����
 * @author  ZMJ
 * @date	6/24/2006 
 *
 * @param	T_GLOBALMEMHandle hMemory : memory allocator handler
 *
 * @return  void
 *
 */
void Ram_DisableAutoLeakTrace(T_GLOBALMEMHandle hMemory);

/**
 * @brief  �ڴ���Ϣ
 * @
 * @memory 
 */
typedef struct tagGLOBALMEMInfo
{
	unsigned short	align;			//�ڴ�������	
	unsigned long	szTotal;		//ȫ�ֶ��ڴ���ܴ�С
	unsigned long	szBlocks;		//ȫ�ֶ��ڴ���ܿ����
	unsigned long   szUsed;			//��ǰʹ�õ��ڴ���ܴ�С
	unsigned long	blkUsed;		//��ǰʹ�õ��ڴ���ܿ���
	unsigned long	szSpare;		//������ʣ��δ������ڴ��С<����ͷ�����ԣ����еȿռ�>
	unsigned long	cntBubble;		//�ڴ������ܸ���<��������ʹ�õ�+�Ѿ��ͷŵ�δ���յ�>
	unsigned short   padblkBubble;	//�����ڴ����ݳ�����SIZE�������ӿռ�ռ�ÿ���
	unsigned short	padszBubble;	//�����ڴ����ݳ�����SIZE�������ӿռ��ֽ���
}GLOBALMEMInfo;	

/**
 * @brief  get global memory info
 * @author  ZMJ
 * @date	6/24/2006 
 *
 * @param	T_GLOBALMEMHandle hMemory : memory allocator handler
 * @param	GLOBALMEMInfo *info : mem info
 *
 * @return  AK_FALSE for not initialize memory else return AK_TRUE
 *
 */
bool Ram_GetRamInfo(T_GLOBALMEMHandle hMemory, GLOBALMEMInfo *info);

/**
 * @brief  get memory info
 * @author  ZMJ
 * @date	6/24/2006 
 *
 * @param	T_GLOBALMEMHandle hMemory : memory allocator handler
 * @param	GLOBALMEMInfo *info : mem info
 *
 * @return  AK_FALSE for not initialize memory else return AK_TRUE
 * &return  wildType information for AK_FALSE to query.
 *
 */
bool Ram_GetPtrInfo(T_GLOBALMEMHandle hMemory, void * var, T_MEMORY_TRACE *map, T_WILD_TYPE *wildType);

/**
 * @brief  get allocable largest memory bubble
 * @note:  �ú���û���Ż�(�ٶ�Զ����alloc)�������Ƶ������ 
 * @author  ZMJ
 * @date	6/12/2008 
 *
 * @param	T_GLOBALMEMHandle hMemory : memory allocator handler
 *
 * @return  allocable largest size
 *
 */
unsigned long  Ram_GetLargestSize_Allocable(T_GLOBALMEMHandle hMemory);



#ifdef __cplusplus
}
#endif


#endif


