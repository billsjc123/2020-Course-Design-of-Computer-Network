#include "MainHeader.h"
#include "MessageConversion.h"
#include "DomainAnalysis.h"
#include "RequestPool.h"



host_item *hosts_list[MAX_HOST_ITEM];
cache_item *cached_list[MAX_CACHED_ITEM];

ReqPool *request_pool;
std::mutex id_mutex, pool_mutex, req_counter_mutex,time_mutex,cache_mutex;
int req_counter = 0, host_counter = 0;
std::thread *dns_consulting_threads[MAX_THREAD];


int main()
{
	dnsRelay();
	return 0;
}

int dnsRelay()
{
	// Initialize, load history data
	int iResult = 0;

	loadHostsFromTxt();

	// Initialize Cached_list
	for (int i = 0; i < MAX_CACHED_ITEM; i++)
	{
		cached_list[i] = (cache_item*)malloc(sizeof(cache_item));
		cached_list[i]->webaddr = (char*)malloc(DEFAULT_BUFLEN);
		cached_list[i]->occupied = false;
	}

	// Initialize, create listen socket
	SOCKET Listen_Socket;
	iResult = initDNSServer(&Listen_Socket);
	if (iResult == 1) return 255;

	// Initialize DNSRequest Pool
	request_pool = (ReqPool*)malloc(sizeof(ReqPool)*MAX_REQ);
	if (request_pool == NULL)
	{
		printf("malloc request_pool failed\n\n");
		exit(100);
	}
	for (int i = 0; i < MAX_REQ; i++)
	{
		request_pool[i].available = true;
	}

	for (int i = 0; i < MAX_THREAD; i++)
	{
		// thread 只能用new？
		dns_consulting_threads[i] = new std::thread(CounsultThreadHandle, UPPER_DNS, Listen_Socket, i);
	}

	printf("Initialize Complete. \n\n");

	do
	{
		char *recvbuf = (char*)malloc(DEFAULT_BUFLEN * sizeof(char));

		struct sockaddr_in clt_addr;
		int clt_addr_len = sizeof(clt_addr);
		if (sizeof(recvbuf) <= 0)
		{
			printf("Length of recvbuf is 0!\n\n");
			exit(100);
		}
		memset(recvbuf, 0, sizeof(recvbuf));

		ZeroMemory(&clt_addr, sizeof(clt_addr));

		// Receive DNS Requests
		if (sizeof(recvbuf) <= 0)
		{
			printf("Length of recvbuf is 0!\n\n");
			exit(100);
		}
		if (recvfrom(Listen_Socket, recvbuf, DEFAULT_BUFLEN, 0, (struct sockaddr*)&clt_addr, &clt_addr_len) == SOCKET_ERROR)
		{
			printf("[Listen_Socket]: recvfrom client error with: %d\n\n", WSAGetLastError());
		}
		else {
			// 有时会莫名其妙收到垃圾报文
			if (strcmp("127.0.0.1", inet_ntoa(clt_addr.sin_addr)) != 0 && strcmp("10.128.211.115", inet_ntoa(clt_addr.sin_addr)))
			{
			
				printf("Receive bad message!\n\n");
				continue;
			}
			printf("[Listen_Socket]: Bytes received from IP(%s): %d\n", inet_ntoa(clt_addr.sin_addr), iResult);
			DNSRequest *new_req = (DNSRequest*)malloc(sizeof(DNSRequest));
			if (new_req == NULL)
			{
				printf("malloc new_req failed\n\n");
				exit(100);
			}
			new_req->packet = unpackDNSPacket(recvbuf);
			new_req->processed = false;
			new_req->client_addr = clt_addr;
			new_req->client_addr_len = clt_addr_len;
			if (addDNSRequestPool(new_req) == -1)
			{
				printf("[Listen_Socket]: Too many requests. Ignore current one.\n\n");
				Sleep(1000);
			}
		}
	} while (true);
}

int initDNSServer(SOCKET *ret_socket)
{
	int iResult = 0;

	// Initalize Winsock
	WSADATA wsaData;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		return 255;
	}

	iResult = 0;
	SOCKET ListenSocket = INVALID_SOCKET;

	//Create a new socket
	ListenSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (ListenSocket == INVALID_SOCKET)
	{
		WSACleanup();
		return 1;
	}

	// 接受任何ip地址发到53端口的报文
	struct sockaddr_in hints;
	hints.sin_family = AF_INET;
	hints.sin_addr.s_addr = INADDR_ANY;
	hints.sin_port = htons(DNS_PORT);

	iResult = ::bind(ListenSocket, (struct sockaddr*)&hints, sizeof(hints));
	if (iResult == SOCKET_ERROR) {
		WSACleanup();
		return 1;
	}

	*ret_socket = ListenSocket;
	return 0;
}

void CounsultThreadHandle(const char* upper_DNS_addr, SOCKET listen_socket, int t_id)
{
	printf("[Consulting Thread %d]: Created.\n\n",t_id);
	char *sendbuf = (char*)malloc(DEFAULT_BUFLEN);
	char *dnsbuf = (char*)malloc(DEFAULT_BUFLEN);
	int iResult = 0;

	//上层DNS服务器的地址
	struct sockaddr_in servaddr;
	ZeroMemory(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(DNS_PORT);
	inet_pton(AF_INET, upper_DNS_addr, &servaddr.sin_addr);
	
	struct sockaddr_in myaddr;
	ZeroMemory(&myaddr, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(10000+t_id);

	// 初始化上层DNS线程
	SOCKET upper_dns_socket = socket(AF_INET, SOCK_DGRAM, 0);
	::bind(upper_dns_socket, (struct sockaddr*)&myaddr, sizeof(myaddr));
	std::thread return_thread = std::thread(UpperThreadHandle, upper_dns_socket, listen_socket, t_id);

	while (1)
	{
		DNSRequest *req = NULL;
		while (req == NULL)
		{
			Sleep(20);
			req = getDNSRequest();
			time_t endTime = time(NULL);

			if (time_mutex.try_lock())
			{
				for (int i = 0; i < MAX_REQ; i++)
				{
					//该请求池有请求
					if (!request_pool[i].available)
					{
						//未被某个线程处理
						if (request_pool[i].req->processed == true)
						{
							if (difftime(endTime, request_pool[i].startTime) > MAX_REQ_TTL)
							{
								DNSRequest* req = request_pool[i].req;
								DNSPacket* recv_packet = req->packet;

								//// 根据ip地址生成发回客户端的字节流
								//int sendbuflen;
								//sendbuf = getDNSResult(recv_packet, req->old_id, 0, BLOCKED, sendbuflen);
								//printf("[Consulting Thread %d]:Start sending result to client\n", t_id);

								//// iResult作为循环判断条件
								//iResult = sendto(listen_socket, sendbuf, sendbuflen, 0, (struct sockaddr*)&(req->client_addr), req->client_addr_len);
								//if (iResult == SOCKET_ERROR)
								//	printf("[Consulting Thread %d]:sendto() failed with error code : %d\n", t_id, WSAGetLastError());
								//else
								//{
								//	printf("[Consulting Thread %d]:Bytes send back to 127.0.0.1: %d\n", t_id, iResult);
								//}
								finishDNSRequest(req->new_id);
							}
							break;
						}
					}
				}
				time_mutex.unlock();
			}

		}
		printf("[Consulting Thread %d]:Got DNSReq %d\n\n",t_id,req->new_id);

		DNSPacket *recv_packet = req->packet;

		UINT32 ip_addr = 0;
		ADDR_TYPE addr_type = getAddrType(recv_packet->p_qpointer[0]->q_qname, &ip_addr);
		printf("[Consulting Thread %d]:Search type finished, type: %d\n\n",t_id,addr_type);

		switch (addr_type)
		{
		case BLOCKED:
		case CACHED:
		{
			// 根据ip地址生成发回客户端的字节流
			int sendbuflen;
			sendbuf = getDNSResult(recv_packet,req->old_id, ip_addr, addr_type, sendbuflen);
			printf("[Consulting Thread %d]:Start sending result to client\n\n", t_id);
			

			// iResult作为循环判断条件
			iResult = sendto(listen_socket, sendbuf, sendbuflen, 0, (struct sockaddr*)&(req->client_addr), req->client_addr_len);
			if (iResult == SOCKET_ERROR)
				printf("[Consulting Thread %d]:sendto() failed with error code : %d\n\n",t_id, WSAGetLastError());
			else
			{
				printf("[Consulting Thread %d]:Bytes send back to 127.0.0.1: %d\n\n",t_id, iResult);
			}
			finishDNSRequest(req->new_id);
		}
		break;
		case ADDR_NOT_FOUND:
		{
			int packet_length;
			ushort p_id = req->new_id;
			recv_packet->p_header->h_id = p_id;
			char* send_string = packDNSPacket(recv_packet, packet_length);
			printf("[Consulting Thread %d]:Start consulting Upper DNS: %s\n\n",t_id, upper_DNS_addr);
			if (sendto(upper_dns_socket, send_string, packet_length, 0, (struct sockaddr*)&servaddr, sizeof(servaddr)) == SOCKET_ERROR)
				printf("[Consulting Thread %d]:sendto() failed with error code : %d\n\n",t_id, WSAGetLastError());
		}
		break;
		}
	}
}

void UpperThreadHandle(SOCKET upper_dns_socket, SOCKET listen_socket, int t_id)
{
	int iResult = 0;
	int sleeptime = 20;
	struct sockaddr_in servaddr;
	int servaddrlen = sizeof(servaddr);
	char *dnsbuf = (char*)malloc(DEFAULT_BUFLEN);
	while (1)
	{
		iResult = recvfrom(upper_dns_socket, dnsbuf, DEFAULT_BUFLEN, 0, (struct sockaddr*)&servaddr, &servaddrlen);
		/*
		 * WSAEWOULDBLOCK：代表接收或者发送的BUFFER满了
		 * 要等待一会才能接收，并没有发生错误
		 * ELSE：传递过程中发生了错误
		 */
		
		if (iResult == SOCKET_ERROR)											
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)											
			{
				Sleep(sleeptime);
				continue;
			}
			else													
			{
				printf("[DNS Thread %d]:! recvfrom_server() failed with error code : %d\n\n",t_id, WSAGetLastError());
				break;
			}
		}
		else
		{
			printf("[DNS Thread %d]:Bytes received from DNS_SERVER: %d\n\n",t_id, DEFAULT_BUFLEN);

			// 得到new_id（the i_th pool）
			int p_id = ntohs(*(ushort*)dnsbuf);
			if (request_pool[p_id].available == true)
			{
				continue;
			}
			DNSPacket* return_pack = unpackDNSPacket(dnsbuf);

			// 获取该报文的ip和域名等信息以存入cached_list中
			if (return_pack->p_rpointer[0]->r_rdata!=NULL && return_pack->p_qpointer[0]->q_qtype==1)
			{
				UINT32* rdata_pointer = (UINT32*)return_pack->p_rpointer[0]->r_rdata;
				UINT32 ip_uint = (*rdata_pointer);
				in_addr inaddr;
				inaddr.S_un.S_addr = ip_uint;
				char* ipaddr = inet_ntoa(inaddr);
				char* webaddr = (char*)malloc(DEFAULT_BUFLEN);
				
				strcpy(webaddr, return_pack->p_qpointer[0]->q_qname);

				// 将不可见的字符转化为.方便对比
				for (int i = 0; i < strlen(webaddr); i++)
				{
					if (webaddr[i] < 0x20)
						webaddr[i] = '.';
					else if (webaddr[i] >= 'A' && webaddr[i] <= 'Z')
					{
						webaddr[i] -= 'A' - 'a';
					}
				}
				printf("[DNS Thread %d]:Domain: %s,IP: %s saved in CACHE\n\n", t_id, webaddr, ipaddr);

				cache_mutex.lock();
				for (int i = 0; i < MAX_CACHED_ITEM; i++)
				{
					if (cached_list[i]->occupied)
					{
						continue;
					}
					cached_list[i]->occupied = true;
					inet_pton(AF_INET, ipaddr, &cached_list[i]->ip_addr);
					strcpy(cached_list[i]->webaddr, webaddr+1);
					break;
				}
				cache_mutex.unlock();
			}

			DNSRequest *req = finishDNSRequest(p_id);
			// 把id改为原来的id
			*(ushort*)dnsbuf = htons(req->old_id);

			// 直接发送给原地址
			iResult = sendto(listen_socket, dnsbuf, DEFAULT_BUFLEN, 0, (struct sockaddr*)&(req->client_addr), req->client_addr_len);
			char* client_ipaddr = inet_ntoa(req->client_addr.sin_addr);
			if (iResult == SOCKET_ERROR)
				printf("[DNS Thread %d]:sendto() failed with error code : %d\n\n",t_id, WSAGetLastError());
			else
			{
				printf("[DNS Thread %d]:Bytes send to %s : %d\n\n",t_id, client_ipaddr,iResult);
			}
		}
	}
}



