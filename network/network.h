#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>

/* Call from the network task, which owns the lwIP context. */
bool network_init(void);
void network_process(void);

#endif
