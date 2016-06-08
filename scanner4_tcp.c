#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include "utils.h"
#include "scanner.h"

static int reader(struct scanner *sc)
{
	int ret;

	ret = recv(sc->rawfd, sc->ibuf, sizeof(sc->ibuf), 0);
	if (ret < 0)
		fatal("recv(3)");

	printf("->\n");
	dump(sc->ibuf, ret);

	return ret;
}

static unsigned short tcp4_checksum(struct scanner *sc, struct tcphdr *tcp)
{
	struct iptmp {
		u_int32_t saddr;
		u_int32_t daddr;
		u_int8_t buf;
		u_int8_t protocol;
		u_int16_t length;
		struct tcphdr tcp;
	} *tmp = (struct iptmp *) sc->cbuf;
	tmp->tcp = *tcp;

	return checksum((uint16_t *) sc->cbuf, sizeof(*tmp));
}

static int writer(struct scanner *sc)
{
	struct sockaddr_in *sin;
	struct tcphdr *tcp;
	struct iphdr *ip;
	int ret;

	printf("<-\n");

	/* IP header. */
	ip = (struct iphdr *) sc->obuf;
	ip->id = htonl(54321); /* randomize. */

	/* TCP header. */
	tcp = (struct tcphdr *)(sc->obuf + 20);
	tcp->th_sport = 0;
	tcp->th_dport = htons(sc->next_port);
	tcp->th_seq = 0;
	tcp->th_ack = 0;
	tcp->th_x2 = 0;
	tcp->th_off = 5;
	tcp->th_flags = TH_SYN;
	tcp->th_win = 0;
	tcp->th_sum = 0;
	tcp->th_urp = 0;
	tcp->th_sum = tcp4_checksum(sc, tcp);

	dump(sc->obuf, sc->olen);

	ret = sendto(sc->rawfd, sc->obuf, sc->olen, 0, sc->dst->ai_addr,
			sc->dst->ai_addrlen);
	if (ret != sc->olen)
		fatal("sendto()");

	if (++sc->next_port > sc->end_port) {
		/* Disable writer event. */
		sc->ev.events = EPOLLIN;
		epoll_ctl(sc->eventfd, EPOLL_CTL_MOD, sc->rawfd, &sc->ev);
		printf("done with sending\n");
	}

	return ret;
}

void scanner_tcp4_init(struct scanner *sc)
{
	struct sockaddr_in *sin;
	struct iphdr *ip;
	int on = 1;
	int ret;

	ret = setsockopt(sc->rawfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));
	if (ret != 0)
		fatal("setsockopt(3)");

	/* TCPv4 specific reader/writer. */
	sc->reader = reader;
	sc->writer = writer;

	/* Prepare the IP header. */
	ip = (struct iphdr *) sc->obuf;
	ip->ihl = 5;
	ip->version = 4;
	ip->tos = 0;
	ip->tot_len = 0;
	ip->frag_off = 0;
	ip->ttl = 255;
	ip->protocol = IPPROTO_TCP;
	ip->check = 0;
	sin = (struct sockaddr_in *) &sc->src;
	ip->saddr = sin->sin_addr.s_addr;
	sin = (struct sockaddr_in *) sc->dst->ai_addr;
	ip->daddr = sin->sin_addr.s_addr;

	/* We only send TCP/IP header portion. */
	sc->olen = sizeof(struct iphdr) + sizeof(struct tcphdr);

	/* Prepare the checksum buffer. */
	struct iptmp {
		u_int32_t saddr;
		u_int32_t daddr;
		u_int8_t buf;
		u_int8_t protocol;
		u_int16_t length;
		struct tcphdr tcp;
	} *tmp = (struct iptmp *) sc->cbuf;
	tmp->saddr = ip->saddr;
	tmp->daddr = ip->daddr;
	tmp->buf = 0;
	tmp->protocol = ip->protocol;
	tmp->length = htons(20);
}