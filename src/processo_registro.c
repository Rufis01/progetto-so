#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>

#include "mappa.h"
#include "log.h"
#include "inline_rw_utils.h"

#define TRENI_MAPPA1 4
#define TRENI_MAPPA2 5
#define MAX_TAPPE 7

/*
static char *tappe[2] =
{
	{"S1","S2","S3","S4","S5","S6","S7","S8"},
	{"MA1","MA2","MA3","MA4","MA5","MA6","MA7","MA8","MA9","MA10","MA11","MA12","MA13","MA14","MA15","MA16"}
};
*/

#ifdef DEBUGMAP
#undef TRENI_MAPPA1
#define TRENI_MAPPA1 1
static char *nomi_mappa1[TRENI_MAPPA1][MAX_TAPPE] =
{
	{"S7","MA13","MA12","MA11","MA10","MA9","S3"}
};
#else
static char *nomi_mappa1[TRENI_MAPPA1][MAX_TAPPE] =
{
	{"S1","MA1","MA2","MA3","MA8","S6"},
	{"S2","MA5","MA6","MA7","MA3","MA8","S6"},
	{"S7","MA13","MA12","MA11","MA10","MA9","S3"},
	{"S4","MA14","MA15","MA16","MA12","S8"},
};
#endif

static char *nomi_mappa2[TRENI_MAPPA2][MAX_TAPPE]  =
{
	{"S2","MA5","MA6","MA7","MA3","MA8","S6"},
	{"S3","MA9","MA10","MA11","MA12","S8"},
	{"S4","MA14","MA15","MA16","MA12","S8"},
	{"S6","MA8","MA3","MA2","MA1","S1"},
	{"S5","MA4","MA3","MA2","MA1","S1"},
};

static int creaSocket(void);
static void servizio(void);
static void accettaConnessioni(int sfd, mappa_id map);
static void creaFiglio(void(*fun)(void *), void *args);
static void trenoLoop(void *args);
static void superLoop(void *args);
static inline uint8_t getNumTreni(mappa_id map);
static uint8_t getNumTappe(char **itinerario);
static void impostaTimer(int sfd, int time);

/* processo_registro richiede come parametro la mappa */
int main(int argc, char **argv)
{
	if(argc != 2)
	{
		exit(EXIT_FAILURE);
	}

	mappa_id map = m_getMappaId(argv[1]);

	if(map == MAPPA_NON_VALIDA)
	{
		exit(EXIT_FAILURE);
	}

	log_init("./log/registro.log");

	int sfd = creaSocket();
	accettaConnessioni(sfd, map);

	log_fini();

	//crea socket AF_UNIX (unica)
	//mettiti in ascolto
	//rispondi n volte con l'itinerario i
	///TODO: muori?

	return 0;

}

static void accettaConnessioni(int sfd, mappa_id map)
{
	struct sockaddr_un clientAddr;
	socklen_t clientLen = sizeof(struct sockaddr_un);
	uint8_t treni = 0;

	impostaTimer(sfd, 20);

	for(;;)
	{
		char buf[64 + 1] = {0};
		int clientFd = accept(sfd, (struct sockaddr *)&clientAddr, &clientLen);

		if(clientFd >= 0)
		{
			LOGD("A client has connected with file descriptor %d\n", clientFd);
		}
		else
		{
			if(errno == EWOULDBLOCK)
			{
				LOGW("No new connection in 20 seconds. Exiting...\n");
				exit(EXIT_SUCCESS);
			}

			LOGE("accept() returned an error and errno was set to %d\n", errno);
			perror("accettaConnessioni");
			continue;
		}

		impostaTimer(clientFd, 5);

		if(readWL(clientFd, buf, sizeof(buf) - 1) == 0)
		{
			LOGW("A client tried to connect, but no command has been recieved within 5 seconds\n");
			LOGD("Closing the connection\n");
			close(clientFd);
			continue;
		}

		if(strcmp(buf, "TRAIN") == 0)
		{
			treni++;
			int args[] = {clientFd, map, treni - 1};
			writeWL(clientFd, "OK", 3);
			LOGD("A client of type TRAIN has connected\n");
			creaFiglio(trenoLoop, args);
		}
		else if(strcmp(buf, "SUPER") == 0)
		{
			int args[] = {clientFd, map};
			writeWL(clientFd, "OK", 3);
			LOGD("A client of type SUPER has connected\n");
			creaFiglio(superLoop, &args);
		}
		else
		{
			LOGW("A client issued an unknown command: %s\n", buf);
			writeWL(clientFd, "418 TEAPOT", 10);
		}
	}
}

static void creaFiglio(void(*fun)(void *), void *args)
{
	int fk = fork();
	if(fk == 0)
	{
		(*fun)(args);
		exit(EXIT_SUCCESS);
	}
	else if(fk < 0)
	{
		perror("fork");
		LOGE("fork() failed\n");
		exit(EXIT_FAILURE);
	}
}

static void trenoLoop(void *args)
{
	///TODO: refactor this abomination

	int fd = ((int *)args)[0];
	mappa_id map_id = (mappa_id)((int *)args)[1];
	uint8_t id = ((int *)args)[2];

	//basically char* mappa[][7]
	char *((*mappa)[7]) = (map_id == MAPPA1 ? nomi_mappa1 : nomi_mappa2);

	impostaTimer(fd, 0);

	for(;;)
	{
		char buf[16 + 1] = {0};
		if(readWL(fd, buf, sizeof(buf) - 1) <= 0)
			break;

		if(strcmp(buf, "ITINERARIO") == 0)
		{
			LOGD("A TRAIN client has issued a ITINERARIO command\n");
			uint8_t numTappe = getNumTappe(mappa[id]);
			writeWL(fd, &id, sizeof(uint8_t));
			writeWL(fd, &numTappe, sizeof(uint8_t));
			for(int i=0; i<numTappe; i++)
			{
				writeWL(fd, (mappa[id])[i], strlen((mappa[id])[i]));
			}
		}
	}
}

static void superLoop(void *args)
{
	///TODO: refactor this abomination

	int fd = ((int *)args)[0];
	mappa_id map_id = (mappa_id)((int *)args)[1];
	uint8_t id = ((int *)args)[2];

	//basically char* mappa[][7]
	char *((*mappa)[7]) = (map_id == MAPPA1 ? nomi_mappa1 : nomi_mappa2);

	impostaTimer(fd, 0);

	for(;;)
	{
		char buf[16 + 1] = {0};

		if(readWL(fd, buf, sizeof(buf) - 1) <= 0)
			break;

		if(strcmp(buf, "TRENI") == 0)
		{
			LOGD("A SUPER client has issued a TRENI command\n");
			int numTreni = getNumTreni(map_id);
			writeWL(fd, (char *)&numTreni, sizeof(int));
		}
		else
		{
			LOGD("A SUPER client as issued an unknown command: %s\n", buf);
		}
	}
}

static int creaSocket(void)
{
	int sfd, r;
	struct sockaddr_un serverAddr =
	{
		.sun_family = AF_UNIX,
		.sun_path = "./sock/registro"
	};

	mkdir("./sock", 0777);

	r = unlink(serverAddr.sun_path);
	LOGD("unlink() returned %d\n", r);

	sfd = socket(AF_UNIX, SOCK_STREAM, 0);
	LOGD("Created socket with file desciptor %d\n", sfd);

	r = bind(sfd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr_un));
	LOGD("bind() returned %d\n", r);
	r = listen(sfd, 5);
	LOGD("listen() returned %d\n", r);

	return sfd;
}

static void impostaTimer(int sfd, int time)
{
	struct timeval rdtimeout =
	{
		.tv_sec = time
	};

	//Imposta un timeout
	if(setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, /*(const char*)*/&rdtimeout, sizeof rdtimeout) < 0)
	{
		LOGE("Impossibile impostare la socket!\n");
		perror("registro_client");
		exit(EXIT_FAILURE);
	}
}

static inline uint8_t getNumTreni(mappa_id map)
{
	switch(map)
	{
		case MAPPA1:
			return TRENI_MAPPA1;
		case MAPPA2:
			return TRENI_MAPPA2;
		default:
			return 0;
	}
}

///TODO: Broken probably!
static uint8_t getNumTappe(char **itinerario)
{
	uint8_t stazioni = 0;
	uint8_t numTappe = 0;

	int i=0;

	do
	{
		if(ISSTAZIONE(itinerario[i]))
		{
			stazioni++;
			LOGD("%s is a station\n", itinerario[i]);
		}
		else
		{
			LOGD("%s is not a station\n", itinerario[i]);
		}
		numTappe++;
		i++;
	} while (stazioni < 2 && i< MAX_TAPPE);

	return numTappe;
}
