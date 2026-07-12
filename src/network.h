#ifndef NETWORK_H
#define NETWORK_H

#include "repo.h"


int network_serve(Repository* repo, int port);

int network_push(Repository* repo, const char* host, const char* port_str, const char* branch);

int network_pull(Repository* repo, const char* host, const char* port_str, const char* branch);

#endif 