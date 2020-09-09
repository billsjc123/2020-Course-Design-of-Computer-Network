#include "MessageConversion.h"

DNSHeader* fromDNSHeader(char* src, char **ret_ptr)
{
	DNSHeader *new_q = (DNSHeader*)malloc(sizeof(DNSHeader));

	int loc_pointer = 0;
	ushort *pointer = (ushort*)src;
	ushort cur_word = htons(*pointer);

	//Get transaction ID
	new_q->h_id = (int)cur_word;

	//Get flags
	cur_word = htons(*(++pointer));
	new_q->h_qr = (bool)((cur_word & 0x8000) >> 1
		);
	new_q->h_opcode = (ushort)((cur_word & 0x7800) >> 11);
	new_q->h_aa = (bool)((cur_word & 0x0400) >> 10);
	new_q->h_tc = (bool)((cur_word & 0x0200) >> 9);
	new_q->h_rd = (bool)((cur_word & 0x0100) >> 8);
	new_q->h_ra = (bool)((cur_word & 0x0080) >> 7);
	new_q->h_rcode = (ushort)((cur_word & 0x000F));
	//new_q->h_flags = cur_word;

	//Get Counts
	cur_word = htons(*(++pointer));
	new_q->h_QDCount = cur_word;
	cur_word = htons(*(++pointer));
	new_q->h_ANCount = cur_word;
	cur_word = htons(*(++pointer));
	new_q->h_NSCount = cur_word;
	cur_word = htons(*(++pointer));
	new_q->h_ARCount = cur_word;

	*ret_ptr = (char*)(++pointer);
	return new_q;
}

DNSQuery *fromDNSQuery(char *src, char **ret_ptr)
{
	int qname_length = 0;
	DNSQuery *new_q = (DNSQuery *)malloc(sizeof(DNSQuery));

	do
		qname_length++;							//Get QueryName length
	while (*(src + qname_length) != '\0');
	char *s = (char*)malloc(qname_length);
	strcpy(s, src);
	new_q->q_qname = s;

	src += (++qname_length);
	ushort *tmp = (ushort*)src;
	new_q->q_qtype = htons(*(tmp++));
	new_q->q_qclass = htons(*tmp);

	*ret_ptr = (char*)(++tmp);
	return new_q;
}

DNSResponse *fromDNSResponse(char* src, char* head, char **ret_ptr)
{
	DNSResponse *new_r = (DNSResponse *)malloc(sizeof(DNSResponse));
	char *s = (char*)malloc(256 * sizeof(char));
	int qname_length = 0;
	char *final_name_dst = src;
	bool name_jumped = false;

	char *name_pointer = src;
	//获取域名
	while (1)
	{
		if (*name_pointer == '\0')
		{
			s[qname_length] = '\0';
			if (name_jumped == false)
				final_name_dst = src + qname_length;
			break;
		}
		if (((*name_pointer) & 0xc0) == 0xc0)
		{
			int new_dst = htons(*((ushort*)name_pointer)) & 0x3f;
			new_dst += (int)head;
			name_jumped = true;
			final_name_dst = name_pointer + 2;
			name_pointer = (char*)new_dst;
			continue;
		}
		if (*name_pointer < 20)
		{
			int tmp_len = *name_pointer++;
			s[qname_length++] = tmp_len;
			for (int i = 0; i < tmp_len; i++)
				s[qname_length++] = *(name_pointer++);
		}
	}

	new_r->r_name = s;

	src = final_name_dst;
	ushort *tmp = (ushort*)src;
	new_r->r_type = htons(*(tmp++));
	new_r->r_class = htons(*(tmp++));
	new_r->r_ttl = htonl(*((int*)tmp));
	tmp += 2;
	new_r->r_rdlength = htons(*(tmp++));

	src = (char*)tmp;
	s = (char*)malloc((new_r->r_rdlength + 1) * sizeof(char));
	memcpy(s, src, new_r->r_rdlength);
	s[new_r->r_rdlength] = '\0';
	new_r->r_rdata = s;

	*ret_ptr = src + new_r->r_rdlength;
	return new_r;
}

char *toDNSHeader(DNSHeader *ret_h)
{
	ushort *tmp_s;
	char* ret_s;
	tmp_s = (ushort*)malloc(13 * sizeof(char));
	ret_s = (char*)tmp_s;
	*(tmp_s++) = ntohs((ushort)ret_h->h_id);

	*tmp_s = 0;
	ushort tags = 0;
	tags |= (ret_h->h_qr << 15);
	tags |= (ret_h->h_opcode << 11);
	tags |= (ret_h->h_aa << 10);
	tags |= (ret_h->h_tc << 9);
	tags |= (ret_h->h_rd << 8);
	tags |= (ret_h->h_ra << 7);
	tags |= (ret_h->h_rcode);
	*(tmp_s++) = ntohs(tags);
	*(tmp_s++) = ntohs(ret_h->h_QDCount);
	*(tmp_s++) = ntohs(ret_h->h_ANCount);
	*(tmp_s++) = ntohs(ret_h->h_NSCount);
	*(tmp_s++) = ntohs(ret_h->h_ARCount);

	*(char*)tmp_s = '\0';
	return ret_s;
}

char *toDNSQuery(DNSQuery *ret_q)
{
	char *ret_s, *tmp_c;
	ushort *tmp_u;
	int tot_length;

	tot_length = strlen(ret_q->q_qname) + 6;
	ret_s = (char*)malloc(tot_length * sizeof(char));
	tmp_c = ret_s;

	//Copy qname to reply message
	strcpy(tmp_c, ret_q->q_qname);
	tmp_c += strlen(ret_q->q_qname);
	*tmp_c = '\0';
	tmp_c++;
	tmp_u = (ushort*)tmp_c;

	*(tmp_u++) = ntohs(ret_q->q_qtype);
	*(tmp_u++) = ntohs(ret_q->q_qclass);

	tmp_c = (char*)tmp_u;
	tmp_c = '\0';
	return ret_s;
}

char *toDNSResponse(DNSResponse *ret_r)
{
	char *ret_s, *tmp_c;
	ushort *tmp_u;
	int tot_length;

	tot_length = strlen(ret_r->r_name) + 11 + ret_r->r_rdlength + 1;

	//rname
	ret_s = (char*)malloc(tot_length * sizeof(char));
	tmp_c = ret_s;
	strcpy(tmp_c, ret_r->r_name);
	tmp_c += strlen(ret_r->r_name);
	*tmp_c = '\0';
	tmp_c++;
	tmp_u = (ushort*)tmp_c;

	//其它ushort和int
	*tmp_u++ = ntohs(ret_r->r_type);
	*tmp_u++ = ntohs(ret_r->r_class);
	*(int*)tmp_u = ntohl(ret_r->r_ttl);
	tmp_u += 2;
	*tmp_u++ = ntohs(ret_r->r_rdlength);

	tmp_c = (char*)tmp_u;
	memcpy(tmp_c, ret_r->r_rdata, ret_r->r_rdlength);

	return ret_s;
}

DNSPacket *unpackDNSPacket(char *buf)
{
	char *cur_ptr = buf, *ret_ptr;

	DNSPacket *dns_packet = (DNSPacket *)malloc(sizeof(DNSPacket));

	// Read DNS Header
	dns_packet->p_header = fromDNSHeader(cur_ptr, &ret_ptr);
	cur_ptr = ret_ptr;

	// Read DNS Query
	for (int i = 0; i < dns_packet->p_header->h_QDCount; i++)
	{
		dns_packet->p_qpointer[i] = fromDNSQuery(cur_ptr, &ret_ptr);
		cur_ptr = ret_ptr;
	}

	// Read DNS Response
	if (dns_packet->p_header->h_ANCount > 0)
	{
		dns_packet->p_rpointer[0] = fromDNSResponse(cur_ptr, buf, &ret_ptr);
		cur_ptr = ret_ptr;
		dns_packet->p_header->h_ANCount = 1;
	}
	else
	{
		dns_packet->p_rpointer[0] = (DNSResponse*)malloc(sizeof(DNSResponse));
		dns_packet->p_rpointer[0]->r_rdata = NULL;
	}
	//bool h_qr = (dns_packet->p_header->h_flags & 0x8000) >> 15;
	dns_packet->p_qr = dns_packet->p_header->h_qr ? Q_RESPONSE : Q_QUERY;

	return dns_packet;
}

char *packDNSPacket(DNSPacket *packet, int &len)
{
	char *new_header = toDNSHeader(packet->p_header);

	//Convert Query part and Header part
	char *ret_string = (char*)malloc(DEFAULT_BUFLEN);
	memcpy(ret_string, new_header, DNS_HEADER_LEN);
	len = DNS_HEADER_LEN;
	if (packet->p_header->h_QDCount == 1)
	{
		char *new_query = toDNSQuery(packet->p_qpointer[0]);
		memcpy(ret_string + len, new_query, strlen(packet->p_qpointer[0]->q_qname) + 5);
		len += strlen(packet->p_qpointer[0]->q_qname) + 5;
	}

	// Convert DNSResponse if needed (packet is a response)
	if (packet->p_qr == Q_RESPONSE && packet->p_header->h_ANCount > 0)
	{
		char *new_response = toDNSResponse(packet->p_rpointer[0]);
		memcpy(ret_string + len, new_response, strlen(packet->p_rpointer[0]->r_name) + 11 + packet->p_rpointer[0]->r_rdlength);
		len += strlen(packet->p_rpointer[0]->r_name) + 11 + packet->p_rpointer[0]->r_rdlength;
	}

	return ret_string;
}