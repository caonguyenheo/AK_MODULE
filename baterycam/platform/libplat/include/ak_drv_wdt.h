#ifndef _AK_WDT_H_
#define _AK_WDT_H_


/**
 * @brief watch dog function open
 * @author Jian_Kui
 * @date 2016-11-15
 * @param unsigned short feedtime:watch dog feed time, feedtime unit:ms, MAX = 304722ms
 * @return int
 * @retval 0 open success
 * @retval -1  open fail
  */
int ak_drv_wdt_open(unsigned int feed_time);


/**
 * @brief watch dog feed
 * @author Jian_Kui
 * @date 2016-11-15
 * @return int
 * @retval 0 feed success
 * @retval -1  feed fail
  */
int ak_drv_wdt_feed(void);


/**
 * @brief watch dog stop
 * @author Jian_Kui
 * @date 2016-11-15
 * @return int
 * @retval 0 closesuccess
 * @retval -1  close fail
  */
int ak_drv_wdt_close(void);

#endif
