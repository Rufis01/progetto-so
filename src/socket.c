#include <string.h>
#include <stdio.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>

#include "socket.h"

#include "log.h"

int creaSocketUnix(const char *path)
{
	int sfd, r;
	struct sockaddr_un serverAddr =
	{
		.sun_family = AF_UNIX,
	};

	if(!path)
	{
		LOGE("Argomento non valido!\n");
		return -1;
	}

	strncpy(serverAddr.sun_path, path, sizeof(serverAddr.sun_path) - 1);
	serverAddr.sun_path[sizeof(serverAddr.sun_path) - 1] = 0;	//Should already be 0, since the struct was initialized, but let's be safe

	mkdir("./sock", 0777);

	r = unlink(serverAddr.sun_path);
	LOGD("unlink() ha restituito %d\n", r);

	sfd = socket(AF_UNIX, SOCK_STREAM, 0);
	LOGD("socket() ha restituito %d\n", sfd);

	r = bind(sfd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr_un));
	LOGD("bind() ha restituito %d\n", r);

	r = listen(sfd, 5);
	LOGD("listen() ha restituito %d\n", r);

	return sfd;
}

int connettiSocketUnix(const char *path)
{
	int sfd;
	struct sockaddr_un serverAddr =
	{
		.sun_family = AF_UNIX,
	};

	if(!path)
	{
		LOGE("Argomento non valido!\n");
		return -1;
	}

	strncpy(serverAddr.sun_path, path, sizeof(serverAddr.sun_path) - 1);
	serverAddr.sun_path[sizeof(serverAddr.sun_path) - 1] = 0;	//Should already be 0, since the struct was initialized, but let's be safe

	//Crea la socket
	sfd = socket(AF_UNIX, SOCK_STREAM, 0 /*default*/);
	if(sfd<0)
	{
		LOGE("Impossibile aprire la socket!\n");
		perror("socket");
		close(sfd);
		return sfd;
	}

	//Apre la socket
	if(connect(sfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	{
		LOGE("Impossibile connettersi alla socket!\n");
		//perror("connect");
		close(sfd);
		return -1;
	}

	return sfd;
}

bool impostaTimer(int sfd, int secondi)
{
	struct timeval rdtimeout =
	{
		.tv_sec = secondi
	};

	if(setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, &rdtimeout, sizeof rdtimeout) < 0)
	{
		LOGE("Impossibile impostare la socket!\n");
		perror("setsockopt");
		return false;
	}

	return true;
}
