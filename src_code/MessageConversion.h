#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include "MainHeader.h"

/**
 * @brief
 *		��srcָ���char*ת��ΪDNSHeader
 * @param
 *		src Դָ��
 * @param
 *		ret_prt ����ָ��
 * @return
 *		DNSHeader*
 * @note
 *		Header���ȹ̶�Ϊ12 Bytes
 *		��������ʱret_ptr���޸�ΪDNSHeader����һ����ַ
 *		src���ᱻ�޸�
 */
DNSHeader *fromDNSHeader(char*, char**);

/**
 * @brief
 *		��srcָ���char*ת��ΪDNSQuery
 * @param
 *		src Դָ��
 * @param
 *		ret_prt ����ָ��
 * @return
 *		DNSQuery*
 * @note
 *		��������ʱret_ptr���޸�ΪDNSQuery����һ����ַ
 *		src���ᱻ�޸�
 */
DNSQuery *fromDNSQuery(char*, char**);

/**
 * @brief
 *		��srcָ���char*ת��ΪDNSResponse
 * @param
 *		src Դָ��
 * @param
 *		head DNS���ĵ�ͷָ��
  * @param
 *		ret_prt ����ָ��
 * @return
 *		DNSReponse*
 * @note
 *		��������ʱret_ptr���޸�ΪDNSReponse����һ����ַ
 *		src���ᱻ�޸�
 */
DNSResponse *fromDNSResponse(char*, char*, char**);

/**
 * @brief
 *		��DNSHeader*ָ�������ת��Ϊ����������ֽ���
 * @param
 *		DNSHeader* Դָ��
 * @return
 *		char *
 * @note
 *		flags���ֵĴ�СΪ2 Bytes�������˰�λ��(&)������λ(<<)�ķ�������
 *		ret_s�����շ��ص�ָ��
 *		tmp_s��ʵ�ʽ����ֶβ�����ָ��
 */
char *toDNSHeader(DNSHeader*);

/**
 * @brief
 *		��DNSQueryr*ָ�������ת��Ϊ����������ֽ���
 * @param
 *		DNSQueryr* Դָ��
 * @return
 *		char *
 * @note
 *		tmp_u_short_pointer��tmp_char_pointerָ���������ͬ��ֻ����ָ��ķ�Χ��ͬ(2bytes �� 1bytes)
 *		qname�ĳ��Ȳ���ȷ����������'\0'��β����Ҫʹ��strlen��ȡ
 */
char *toDNSQuery(DNSQuery*);

/**
 * @brief
 *		��DNSResponse*ָ�������ת��Ϊ����������ֽ���
 * @param
 *		DNSResponse* Դָ��
 * @return
 *		char *
 * @note
 *		ʹ�÷�����toDNSQuery()��ͬ
 */
char *toDNSResponse(DNSResponse*);

/**
 * @brief
 *		������������ֽ���ָ�������ת��ΪDNSPacket
 * @param
 *		char* Դָ��
 * @return
 *		DNSPacket*
 * @note
 *		����������fromDNSHeader()��fromDNSQuery()��fromDNSResponse()
 *		����DNS���ĸ�ʽ����ת��
 *		����ʵ����Ҫ����������Query��Response����ת����ֻ�ֱ�Ե�һ������ת��
 */
DNSPacket *unpackDNSPacket(char *);

/**
 * @brief
 *		��DNSPacket*ָ�������ת��Ϊ����������ֽ���
 * @param
 *		DNSPacket* Դָ��
 * @param
 *		int& �ֽ�������
 * @return
 *		DNSPacket*
 * @note
 *		unpackDNSPacket�������
 */
char *packDNSPacket(DNSPacket *, int&);