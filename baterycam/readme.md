1.Compilation Steps

  1) Decompress uclibc-ak.tar.gz to working directory

  2) Set compilation configuration
   export CROSS_PATH=/opt/uclibc-ak
   export PATH=$PATH:$CROSS_PATH/bin/

  3) Decompress sky39ev200_svn48602.tar.gz SDK to working directory

  4) Goto sdk. Run make


2.Explanation on directory
  kernel     - kernel, storage system, RAM, driver
  platform   - app
   --apps     
   --libapp   
   --libmpi   - lib for multimedia
   --libplat  - platform middleware
  tools      - burn tool

3.Burn Steps
  1)On PC, Run tools\burn_tool\BurnTool.exe
  2)Press and hold BOOT pin on eval board. Then apply power or press reset button.
    Observe the burn status will change from GREEN to YELLOW.
  3) Press Start button. You will see progress bar updating.
  4) Burn completed.
  

4.Instruction to make adjustment

 1). After eval board is powered up, UART will show sys>
 2). Enter help ,  you will get list of help :
sys> help
system information:
chip = AK3918
ver  = V200
pll  = 400Mhz
asic = 200Mhz
cpu  = 400Mhz
mem  = 200Mhz
ram  = 64MBytes

command list:
utils		: utils tools
aidemo		: ai module demo
aodemo		: audio out module demo
detectdemo	: detect test module
filedemo	: file demo module
leddemo		: led module demo
spidemo		: spi module demo
threaddemo	: thread demo module
uart2demo	: uart2 module demo
videmo		: vi module demo
wdtdemo		: watchdog test module
uvcdemo		: uvc demo module
netdemo		: netdemo
wifidemo	: wifi test module
vencdemo	: encode demo module
help		: :)    
   NOTE: when you enter help, you will get detail of what each command can do

 3)Enter aidemo, you can get more detail info.
 


5.Instruction related to doorbell
1)trunk/Makfile 
   #DEMODIR += platform/apps/ring_call
2)
3)There are demo files in every level. Every demo can operate through UART.
4)plat\demo 
  ak_ai_demo.c      demo to record
  ak_ao_demo.c      demo to playback audio
  ak_spi_demo.c     demo to transfer via spi
  ak_uart2_demo.c   demo to use UART 2 for transfer
5)libmpi\demo 
  ak_venc_demo.c    Get camera data, encode, make it as stream
NOTE: current file system supports ansi only. This is because rtc not supported yet. 
