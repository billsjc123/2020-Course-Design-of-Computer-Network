#include "DomainAnalysis.h"
#include "MessageConversion.h"
using namespace std;

extern host_item *hosts_list[];
extern int host_counter;

void loadHostsFromTxt()
{
	// Prepare reading
	FILE *fp = fopen(HOST_FILE_LOC, "r");
	if (fp == NULL)
	{
		printf("dnsrelay.txt open error...\n");
		exit(1);
	}

	// Start reading 
	char ipaddr[DEFAULT_BUFLEN];
	char domain[DEFAULT_BUFLEN];
	int cnt = 0;
	while (!feof(fp))
	{
		if (cnt == 214)
			cnt = cnt;
		fgets(ipaddr, DEFAULT_BUFLEN, fp);
		for (int i = 0; i < DEFAULT_BUFLEN; i++)
		{
			if (ipaddr[i] == ' ')
			{
				ipaddr[i] = '\0';
				strcpy(domain, ipaddr + i + 1);
				if (domain[strlen(domain) - 1] == '\n')
					domain[strlen(domain) - 1] = '\0';
				else
					domain[strlen(domain)] = '\0';
				break;
			}
		}
		hosts_list[cnt] = (host_item *)malloc(sizeof(host_item));
		hosts_list[cnt]->webaddr = (char*)malloc(DEFAULT_BUFLEN);
		inet_pton(AF_INET, ipaddr, &hosts_list[cnt]->ip_addr);
		strcpy(hosts_list[cnt]->webaddr, domain);

		if (hosts_list[cnt]->ip_addr == 0)
			hosts_list[cnt]->type = BLOCKED;
		else
			hosts_list[cnt]->type = CACHED;
		cnt++;

	}
	host_counter = cnt - 1;
	printf("load %d host from %s successfully\n", cnt, HOST_FILE_LOC);
	fclose(fp);
}

/* 
 * getAddrType tells if the searched address is in host list
 * returns ADDR_TYPE (ADDR_CACHED, ADDR_BLOCKED, ADDR_NOT_FOUND)
 * returns real ip if found, 0.0.0.0 if not found.
 */

ADDR_TYPE getAddrType(char *addr, UINT32 *ip)
{
	int i;
	*ip = 0x0;
	char *tmp_addr = (char*)malloc(DEFAULT_BUFLEN);
	strcpy(tmp_addr, addr);

	// 将不可见的字符转化为.方便对比
	for(i = 0; i < strlen(addr); i++)
	{
		if (tmp_addr[i] < 0x20)
			tmp_addr[i] = '.';
		else if (tmp_addr[i] >= 'A'&&tmp_addr[i] <= 'Z')
		{
			tmp_addr[i] -= 'A' - 'a';
		}
	}

	printf("[Consulting Thread]:get domain from DNSPacket : %s\n", tmp_addr);

	// 从host列表中找到ip
	// 返回域名类型ADDR_TYPE
	/*
	 * 子串查找函数
	 * char *strstr(const char *haystack, const char *needle)
	 *  - haystack -- 要被检索的 C 字符串。
	 *  - needle -- 在 haystack 字符串内要搜索的小字符串。
	 *  - 该函数返回在 haystack 中第一次出现 needle 字符串的位置，如果未找到则返回 null。
	 */
	for(i = 0; i < host_counter; i++)
	{
		if (strstr(tmp_addr, hosts_list[i]->webaddr))
		{
			*ip = htonl(hosts_list[i]->ip_addr);
			if(*ip != 0)
				return CACHED;
			else
				return BLOCKED;
		}
	}
	return ADDR_NOT_FOUND;
}

char *getDNSResult(DNSPacket *ori_packet,int old_id, UINT32 ip_addr, ADDR_TYPE addr_type, int &sendbuflen)
{
	DNSPacket *ret_packet = (DNSPacket *)malloc(sizeof(DNSPacket));
	DNSHeader *ret_header = (DNSHeader *)malloc(sizeof(DNSHeader));
	DNSQuery *ret_query = ori_packet->p_qpointer[0];
	DNSResponse *ret_response = (DNSResponse *)malloc(sizeof(DNSResponse));
	ushort ret_id;

	if(addr_type == BLOCKED)
	{
		//Construct new DNSHeader
		ret_header->h_id = ori_packet->p_header->h_id;
		ret_header->h_qr = 1;
		ret_header->h_opcode = ori_packet->p_header->h_opcode;
		ret_header->h_aa = 0;
		ret_header->h_tc = 0;
		ret_header->h_rd = 1;
		ret_header->h_ra = 1;
		ret_header->h_rcode = 3;
		ret_header->h_QDCount = 0;
		ret_header->h_ANCount = 0;
		ret_header->h_NSCount = 0;
		ret_header->h_ARCount = 0;
	
		// Form new packet
		ret_packet->p_header = ret_header;
		ret_packet->p_qpointer[0] = NULL;
		ret_packet->p_rpointer[0] = NULL;
		ret_packet->p_qr = Q_RESPONSE;
	}
	else
	{
		//Construct new DNSResponse
		ret_response->r_name = ori_packet->p_qpointer[0]->q_qname;
		ret_response->r_type = 1;
		ret_response->r_class = ori_packet->p_qpointer[0]->q_qclass;
		ret_response->r_ttl = 0x100;
		ret_response->r_rdlength = 4;
		ret_response->r_rdata = (char*)malloc(sizeof(UINT32) + 1);
		*(UINT32*)(ret_response->r_rdata) = htonl(ip_addr);

		//Construct new DNSHeader
		ret_header->h_id = ori_packet->p_header->h_id;
		ret_header->h_qr = 1;
		ret_header->h_opcode = ori_packet->p_header->h_opcode;
		ret_header->h_aa = 0;
		ret_header->h_tc = 0;
		ret_header->h_rd = 1;
		ret_header->h_ra = 1;
		ret_header->h_rcode = 0;
		ret_header->h_QDCount = 1;
		ret_header->h_ANCount = 1;
		ret_header->h_NSCount = 0;
		ret_header->h_ARCount = 0;
	
		// Form new packet
		ret_packet->p_header = ret_header;
		ret_packet->p_qpointer[0] = ret_query;
		ret_packet->p_rpointer[0] = ret_response;
		ret_packet->p_qr = Q_RESPONSE;
	}

	ret_packet->p_header->h_id = old_id;

	char* sendbuf = (char*)malloc(DEFAULT_BUFLEN);
	sendbuf = packDNSPacket(ret_packet, sendbuflen);

	return sendbuf;
}