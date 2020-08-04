#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include "MainHeader.h"

/**
 * @brief
 *		将src指向的char*转化为DNSHeader
 * @param
 *		src 源指针
 * @param
 *		ret_prt 返回指针
 * @return
 *		DNSHeader*
 * @note
 *		Header长度固定为12 Bytes
 *		函数结束时ret_ptr被修改为DNSHeader的下一个地址
 *		src不会被修改
 */
DNSHeader *fromDNSHeader(char*, char**);

/**
 * @brief
 *		将src指向的char*转化为DNSQuery
 * @param
 *		src 源指针
 * @param
 *		ret_prt 返回指针
 * @return
 *		DNSQuery*
 * @note
 *		函数结束时ret_ptr被修改为DNSQuery的下一个地址
 *		src不会被修改
 */
DNSQuery *fromDNSQuery(char*, char**);

/**
 * @brief
 *		将src指向的char*转化为DNSResponse
 * @param
 *		src 源指针
 * @param
 *		head DNS报文的头指针
  * @param
 *		ret_prt 返回指针
 * @return
 *		DNSReponse*
 * @note
 *		函数结束时ret_ptr被修改为DNSReponse的下一个地址
 *		src不会被修改
 */
DNSResponse *fromDNSResponse(char*, char*, char**);

/**
 * @brief
 *		将DNSHeader*指向的内容转化为网络二进制字节流
 * @param
 *		DNSHeader* 源指针
 * @return
 *		char *
 * @note
 *		flags部分的大小为2 Bytes，采用了按位与(&)和左移位(<<)的方法构造
 *		ret_s是最终返回的指针
 *		tmp_s是实际进行字段操作的指针
 */
char *toDNSHeader(DNSHeader*);

/**
 * @brief
 *		将DNSQueryr*指向的内容转化为网络二进制字节流
 * @param
 *		DNSQueryr* 源指针
 * @return
 *		char *
 * @note
 *		tmp_u_short_pointer和tmp_char_pointer指向的内容相同，只不过指向的范围不同(2bytes 和 1bytes)
 *		qname的长度不能确定，但是以'\0'结尾，需要使用strlen获取
 */
char *toDNSQuery(DNSQuery*);

/**
 * @brief
 *		将DNSResponse*指向的内容转化为网络二进制字节流
 * @param
 *		DNSResponse* 源指针
 * @return
 *		char *
 * @note
 *		使用方法与toDNSQuery()相同
 */
char *toDNSResponse(DNSResponse*);

/**
 * @brief
 *		将网络二进制字节流指向的内容转化为DNSPacket
 * @param
 *		char* 源指针
 * @return
 *		DNSPacket*
 * @note
 *		函数调用了fromDNSHeader()、fromDNSQuery()和fromDNSResponse()
 *		根据DNS报文格式进行转换
 *		根据实际需要，不对所有Query和Response进行转换，只分别对第一个进行转换
 */
DNSPacket *unpackDNSPacket(char *);

/**
 * @brief
 *		将DNSPacket*指向的内容转化为网络二进制字节流
 * @param
 *		DNSPacket* 源指针
 * @param
 *		int& 字节流长度
 * @return
 *		DNSPacket*
 * @note
 *		unpackDNSPacket的逆过程
 */
char *packDNSPacket(DNSPacket *, int&);