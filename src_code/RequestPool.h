#pragma once
#include "MainHeader.h"

/**
 * @brief
 *		��������л�ȡһ������
 * @param
 *		void
 * @return
 *		DNSRequest*
 * @note
 *		ͨ��available�ж�
 */
DNSRequest* getDNSRequest();

/**
 * @brief
 *		������������һ������
 * @param
 *		DNSRequest*
 * @return
 *		int ��������е�λ��
 * @note
 */
int addDNSRequestPool(DNSRequest *);

/**
 * @brief
 *		���������ɾ��һ������
 * @param
 *		int ��������е�λ��
 * @return
 *		DNSRequest* �������ָ��
 * @note
 */
DNSRequest* finishDNSRequest(int);