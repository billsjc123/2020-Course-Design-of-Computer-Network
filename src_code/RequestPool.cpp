#include "RequestPool.h"

extern ReqPool *request_pool;
extern std::mutex id_mutex, pool_mutex, req_counter_mutex;

DNSRequest* finishDNSRequest(int new_id)
{
	DNSRequest* req;
	std::lock_guard<std::mutex> pool_guard(pool_mutex);
	req = request_pool[new_id].req;
	request_pool[new_id].available = true;
	return req;
}

DNSRequest* getDNSRequest()
{
	DNSRequest* req = NULL;
	if (pool_mutex.try_lock())
	{
		for (int i = 0; i < MAX_REQ; i++)
		{
			if (!request_pool[i].available)
			{
				if (request_pool[i].req->processed == false)
				{
					req = request_pool[i].req;
					request_pool[i].req->processed = true;
					break;
				}
			}
		}
		pool_mutex.unlock();
	}
	return req;
}

int addDNSRequestPool(DNSRequest *req)
{
	std::lock_guard<std::mutex> pool_guard(pool_mutex);
	for (int i = 0; i < MAX_REQ; i++)
	{
		if (request_pool[i].available)
		{
			request_pool[i].available = false;
			req->old_id = req->packet->p_header->h_id;
			req->new_id = i;
			request_pool[i].req = req;
			return i;
		}
	}
	return -1;
}