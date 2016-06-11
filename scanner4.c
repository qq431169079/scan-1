#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "utils.h"
#include "scanner.h"
#include "scanner4_tcp.h"

static char addr[INET_ADDRSTRLEN];

void scanner4_init_const(struct scanner *sc)
{
}

int scanner4_init(struct scanner *sc)
{
	struct sockaddr_in *sin;
	struct iphdr *ip;
	int on = 1;
	int ret;

	ret = setsockopt(sc->rawfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));
	if (ret != 0)
		fatal("setsockopt(IP_HDRINCL)");

	/* Prepare the IP header. */
	ip = (struct iphdr *) sc->obuf;
	ip->ihl = 5;
	ip->version = 4;
	ip->tos = 0;
	ip->tot_len = 0;
	ip->frag_off = 0;
	ip->ttl = 255;
	ip->protocol = sc->dst->ai_protocol;
	ip->check = 0;
	sin = (struct sockaddr_in *) &sc->src;
	ip->saddr = sin->sin_addr.s_addr;
	sin = (struct sockaddr_in *) sc->dst->ai_addr;
	ip->daddr = sin->sin_addr.s_addr;

	inet_ntop(sc->dst->ai_family, &ip->saddr, addr, sizeof(addr));
	debug("Send from %s\n", addr);

	switch (sc->dst->ai_protocol) {
	case IPPROTO_TCP:
		ret = scanner4_tcp_init(sc);
		break;
	default:
		warn("TCP is the only supported protocol in IPv4\n");
		ret = -1;
	}
	return ret;
}