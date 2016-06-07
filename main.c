#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "utils.h"

static const int start_port = 0;
static const int end_port = 65535;

struct scanner {
	/* Sockets. */
	int eventfd;
	int rawfd;

	/* Address family and the protocol. */
	int family;
	int proto;

	/* Scanning port info. */
	int next_port;
	int start_port;
	int end_port;

	/* Reader and writer of the raw socket. */
	int (*reader)(struct scanner *sc);
	int (*writer)(struct scanner *sc);
};

static inline void reader(struct scanner *sc)
{
	if (sc->reader)
		(*sc->reader)(sc);
}

static inline void writer(struct scanner *sc)
{
	if (sc->writer)
		(*sc->writer)(sc);
}

static int __reader(struct scanner *sc)
{
	printf("reader\n");
	return 0;
}

static int __writer(struct scanner *sc)
{
	if (++sc->next_port > sc->end_port) {
		struct epoll_event ev;

		ev.events = EPOLLIN;
		ev.data.fd = sc->rawfd;
		ev.data.ptr = (void *)sc;
		epoll_ctl(sc->eventfd, EPOLL_CTL_MOD, sc->rawfd, &ev);
		printf("done with sending\n");
	}
	return 0;
}

int init(struct scanner *sc, int family, int proto)
{
	struct epoll_event ev;
	int ret;

	sc->eventfd = epoll_create1(0);
	if (sc->eventfd == -1)
		fatal("epoll_create1(2)");

	sc->family = family;
	sc->proto = proto;
	sc->rawfd = socket(sc->family, SOCK_RAW, sc->proto);
	if (sc->rawfd == -1)
		fatal("socket(2)");

	/* We'll set this later. */
	sc->start_port = start_port;
	sc->end_port = end_port;
	sc->next_port = sc->start_port;
	sc->reader = __reader;
	sc->writer = __writer;

	/* Register it to the event manager. */
	ev.events = EPOLLIN|EPOLLOUT;
	ev.data.fd = sc->rawfd;
	ev.data.ptr = (void *)sc;
	ret = epoll_ctl(sc->eventfd, EPOLL_CTL_ADD, sc->rawfd, &ev);
	if (ret == -1)
		fatal("epoll_ctl(2)");

	return sc->eventfd;
}

void term(struct scanner *sc)
{
	if (sc->eventfd == -1)
		return;

	if (sc->rawfd != -1) {
		epoll_ctl(sc->eventfd, EPOLL_CTL_DEL, sc->rawfd, NULL);
		close(sc->rawfd);
	}
	close(sc->eventfd);
	sc->eventfd = sc->rawfd = -1;
}

int main(int argc, char *argv[])
{
	struct scanner sc;
	int eventfd;

	/* Initialize scanner. */
	eventfd = init(&sc, AF_INET, IPPROTO_TCP);

	for (;;) {
		struct epoll_event ev;
		struct scanner *scp;
		int nfds;

		nfds = epoll_wait(eventfd, &ev, 1, -1);
		if (nfds == -1)
			fatal("epoll_wait(2)");

		scp = (struct scanner *) ev.data.ptr;
		if (ev.events & EPOLLIN)
			reader(scp);
		if (ev.events & EPOLLOUT)
			writer(scp);
	}
	term(&sc);

	exit(EXIT_SUCCESS);
}
