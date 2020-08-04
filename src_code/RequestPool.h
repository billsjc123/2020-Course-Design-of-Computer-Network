#pragma once
#include "MainHeader.h"

/**
 * @brief
 *		从请求池中获取一个请求
 * @param
 *		void
 * @return
 *		DNSRequest*
 * @note
 *		通过available判定
 */
DNSRequest* getDNSRequest();

/**
 * @brief
 *		向请求池里添加一个请求
 * @param
 *		DNSRequest*
 * @return
 *		int 在请求池中的位置
 * @note
 */
int addDNSRequestPool(DNSRequest *);

/**
 * @brief
 *		从请求池里删除一个请求
 * @param
 *		int 在请求池中的位置
 * @return
 *		DNSRequest* 该请求的指针
 * @note
 */
DNSRequest* finishDNSRequest(int);