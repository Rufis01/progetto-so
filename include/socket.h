#ifndef SOCKET_H
#define SOCKET_H

#include <stdbool.h>

int creaSocketUnix(const char *path);
int connettiSocketUnix(const char *path);
bool impostaTimer(int sfd, int secondi);

#endif
