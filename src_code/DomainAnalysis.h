#pragma once

#include "MainHeader.h"

typedef unsigned int UINT32;
enum ADDR_TYPE { BLOCKED = 100, CACHED, ADDR_NOT_FOUND };


/**
 * host表项
 * ip_addr :				ip地址
 * webaddr :				域名
 * type :
 */
typedef struct host_item
{
	UINT32 ip_addr;
	char* webaddr;
	ADDR_TYPE type;
}host_item;

/**
 * cache表项
 * ip_addr :				ip地址
 * webaddr :				域名
 * ttl :					该表项的生命周期
 * occupied:				该表现是否被使用
 */
typedef struct cache_item
{
	UINT32 ip_addr;
	char* webaddr;
	int ttl;
	int occupied;
}cache_item;



/**
 * @brief
 *		从dnsrelay.txt中读取ipaddr，domain，存入hosts_list中
 * @param
 *		void
 * @return
 *		void
 * @note
 *		注意dnsrelay.txt中不能有多余的空行，部分域名返回的ip会出现错误
 *		每次读取一整行，再根据空格分开ip和域名
 */
void loadHostsFromTxt();

/**
 * @brief
 *		根据域名确定域名类型
 * @param
 *		addr 域名
 * @param
 *		ip
 * @return
 *		ADDR_TYPE 域名类型	 	
 * @note
 */
ADDR_TYPE getAddrType(char *, UINT32 *);

/**
 * @brief
 *		根据域名类型确定发送给客户端的字节流
 * @param
 *		ori_packet	从客户端接收到的报文
 * @param
 *		old_id		原来的id
 * @param
 *		ip_addr		ip地址
 * @param
 *		addr_type	域名类型
 * @param
 *		sendbuflen	发送字长
 * @return
 *		char *		发送给客户端的字节流
 * @note
 */
char *getDNSResult(DNSPacket *ori_packet,  int old_id, UINT32 ip_addr, ADDR_TYPE addr_type, int &sendbuflen);

