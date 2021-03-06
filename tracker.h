#ifndef _TRACKER_H
#define _TRACKER_H

#include "utils.h"

typedef enum { UNKNOWN, INIT, CLOSED, OPEN } port_status_t;

/* Port tracker. */
struct tracker {
	/* Address of the target host. */
	char *addr;

	/* Start and end of the scanned port. */
	unsigned short begin;
	unsigned short end;

	/* Port tracker. */
	unsigned short next;

	/* Open port status. */
	port_status_t status[UINT16_MAX + 1];
};

/* Inlines. */
static inline void tracker_set_open(struct tracker *t,
		const unsigned short port)
{
	if (port)
		if (t->status[port] == INIT) {
			t->status[port] = OPEN;
			info("Port %d is open on %s\n", port, t->addr);
		}
}

static inline void tracker_open_all(struct tracker *t)
{
	int port;

	for (port = t->begin; port <= t->end; port++)
		t->status[port] = OPEN;
}

static inline void tracker_set_closed(struct tracker *t,
		const unsigned short port)
{
	if (port)
		if (t->status[port] == INIT || t->status[port] == OPEN) {
			t->status[port] = CLOSED;
			info("Port %d is closed on %s\n", port, t->addr);
		}
}

static inline const port_status_t tracker_status(const struct tracker *t,
		const unsigned short port)
{
	return t->status[port];
}

/* Prototypes. */
void tracker_print(const struct tracker *t);
void tracker_init(struct tracker *t, const unsigned short begin_port,
		const unsigned short end_port, char *const addr);
void tracker_term(struct tracker *t);

#endif /* _TRACKER_H */
