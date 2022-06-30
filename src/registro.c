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
#include "socket.h"

#define TRENI_MAPPA1 4
#define TRENI_MAPPA2 5
#define MAX_TAPPE 7

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

static void accettaConnessioni(int sfd, mappa_id map);
static void creaFiglio(void(*fun)(void *), void *args);
static void trenoLoop(void *args);
static void superLoop(void *args);
static uint8_t getNumTreni(mappa_id map);
static uint8_t getNumTappe(char **itinerario);
static void writeItinerario(int fd, char **itinerario, int tid);
static void writeMappa(int fd, mappa_id map_id);

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

	int sfd = creaSocketUnix("./sock/registro");
	accettaConnessioni(sfd, map);

	log_fini();

	return 0;
}

static void accettaConnessioni(int sfd, mappa_id map)
{
	uint8_t treni = 0;

	impostaTimer(sfd, 20);

	for(;;)
	{
		char buf[64 + 1] = {0};
		int clientFd = accept(sfd, NULL, NULL);

		if(clientFd >= 0)
		{
			LOGD("Un client si e' connesso con file descriptor %d\n", clientFd);
		}
		else
		{
			if(errno == EWOULDBLOCK)
			{
				LOGW("Nessuna connessione in 20 secondi. Esco...\n");
				exit(EXIT_SUCCESS);
			}

			LOGE("accept() ha restituito un errore; errno e' stata impostata a %d\n", errno);
			perror("accept");
			continue;
		}

		impostaTimer(clientFd, 5);

		if(readWL(clientFd, buf, sizeof(buf) - 1) == 0)
		{
			LOGW("Un client ha tentato di connettersi, ma non ha inviato niente per 5 secondi!\n");
			LOGD("Chiudo la connessione...\n");
			close(clientFd);
			continue;
		}

		if(strcmp(buf, "TRAIN") == 0)
		{
			treni++;
			int args[] = {clientFd, map, treni - 1};
			writeWL(clientFd, "OK", 3);
			LOGD("Un client di tipo TRAIN si e' connesso\n");
			creaFiglio(trenoLoop, args);
		}
		else if(strcmp(buf, "SUPER") == 0)
		{
			int args[] = {clientFd, map};
			writeWL(clientFd, "OK", 3);
			LOGD("Un client di tipo SUPER si e' connesso\n");
			creaFiglio(superLoop, args);
		}
		else
		{
			LOGW("Un client ha inoltrato un comando sconosciuto: %s\n", buf);
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
		LOGF("fork() non ha avuto successo!\n");
		exit(EXIT_FAILURE);
	}
}

static void trenoLoop(void *args)
{

	int fd = ((int *)args)[0];
	mappa_id map_id = (mappa_id)((int *)args)[1];
	uint8_t id = ((int *)args)[2];

	//char *mappa[?][7]
	char *((*mappa)[7]) = (map_id == MAPPA1 ? nomi_mappa1 : nomi_mappa2);

	impostaTimer(fd, 0);

	for(;;)
	{
		char buf[16 + 1] = {0};
		if(readWL(fd, buf, sizeof(buf) - 1) <= 0)
			break;

		if(strcmp(buf, "ITINERARIO") == 0)
		{
			LOGD("Un client TRAIN ha inoltrato un comando ITINERARIO\n");
			writeItinerario(fd, mappa[id], id);
		}
	}
}

static void superLoop(void *args)
{
	int fd = ((int *)args)[0];
	mappa_id map_id = (mappa_id)((int *)args)[1];


	impostaTimer(fd, 0);

	for(;;)
	{
		char buf[16 + 1] = {0};

		if(readWL(fd, buf, sizeof(buf) - 1) <= 0)
			break;

		if(strcmp(buf, "TRENI") == 0)
		{
			LOGD("Un client SUPER ha inoltrato un comando TRENI\n");
			int numTreni = getNumTreni(map_id);
			writeWL(fd, (char *)&numTreni, sizeof(int));
		}
		else if(strcmp(buf, "MAPPA") == 0)
		{
			LOGD("Un client SUPER ha inoltrato un comando MAPPA\n");
			writeMappa(fd, map_id);
		}
		else
		{
			LOGD("Un client SUPER ha inoltrato un comando sconosciuto: %s\n", buf);
		}
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

static uint8_t getNumTappe(char **itinerario)
{
	uint8_t stazioni = 0;
	uint8_t numTappe = 0;

	int i=0;

	do
	{
		if(ISSTAZIONE(itinerario[i]))
			stazioni++;
		numTappe++;
		i++;
	} while (stazioni < 2 && i< MAX_TAPPE);

	return numTappe;
}

static void writeItinerario(int fd, char **itinerario, int tid)
{
	uint8_t numTappe = getNumTappe(itinerario);
	writeWL(fd, (char *)&tid, sizeof(uint8_t));
	writeWL(fd, (char *)&numTappe, sizeof(uint8_t));
	for(int i=0; i<numTappe; i++)
	{
		writeWL(fd, itinerario[i], strlen(itinerario[i]));
	}
}

static void writeMappa(int fd, mappa_id map_id)
{
	char *((*mappa)[7]) = (map_id == MAPPA1 ? nomi_mappa1 : nomi_mappa2);
	uint8_t treni = getNumTreni(map_id);

	writeWL(fd, (char *)&treni, sizeof(uint8_t));

	for(uint8_t i=0; i<treni;i++)
	{
		writeItinerario(fd, mappa[i], i);
	}
}