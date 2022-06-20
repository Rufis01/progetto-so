#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <poll.h>

#include "log.h"
#include "mappa.h"
#include "inline_rw_utils.h"
#include "socket.h"
#include "registro_client.h"

struct stato_treni
{
	int n;
	int *fd;
	struct posizione *pos;
	struct posizione *prevpos;
};

static struct stato_treni *accettaConnessioni(int sfd);
static void servizio(struct stato_treni *stato, mappa_id mappa);

static int availFd(struct pollfd *clients, int numClients, short events);
static inline void pollSetUp(struct stato_treni *stato, struct pollfd *pfd);
static inline int getTrainId(struct stato_treni *stato, int fd);
static bool checkAuth(struct stato_treni *stato, struct mappa *mappa, struct posizione *pos, uint8_t tid, const char *buf);
static inline void updateStato(struct stato_treni *stato, struct posizione *pos, uint8_t tid);

/* rbc richiede come parametro la mappa */
int main(int argc, char **argv)
{
	if(argc != 2)
	{
		printf("%d\n", argc);
		exit(EXIT_FAILURE);
	}

	mappa_id map = m_getMappaId(argv[1]);

	if(map == MAPPA_NON_VALIDA)
	{
		printf("%s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	log_init("./log/rbc.log");
	rc_init(false);

	int sfd = creaSocketUnix("./sock/rbc");

	struct stato_treni *stato = accettaConnessioni(sfd);

	servizio(stato, map);

	///TODO:free stato
	rc_fini();

	log_fini();
}

static struct stato_treni *accettaConnessioni(int sfd)
{
	int numTreni = rc_getNumeroTreni();

	struct stato_treni *stato = malloc(sizeof(struct stato_treni));
	stato->fd = calloc(numTreni, sizeof(int));
	stato->pos = calloc(numTreni, sizeof(struct posizione));
	stato->prevpos = calloc(numTreni, sizeof(struct posizione));
	stato->n = numTreni;

	for(int i=0; i<numTreni; i++)
	{
		int clientFd = accept(sfd, NULL, NULL);

		if(clientFd >= 0)
		{
			uint8_t trainId;
			readWL(clientFd, &trainId, sizeof(uint8_t));
			writeWL(clientFd, "OK", sizeof("OK"));
			stato->fd[trainId] = clientFd;
			LOGD("Train %d has connected with file descriptor %d\n", trainId, clientFd);
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
	}

	LOGW("WARNING! This is potentially buggy!\n");
	close(sfd);

	return stato;
}

///TODO: Dividere??
static void servizio(struct stato_treni *stato, mappa_id mappa)
{
	struct mappa *map = rc_getMappa();

	uint8_t numStazioni = map_getNumeroStazioni(map);
	uint8_t numBoe = map_getNumeroBoe(map);

	uint8_t *segmenti[2];
	
	segmenti[0] = calloc(numBoe, sizeof(uint8_t));
	segmenti[1] = calloc(numStazioni, sizeof(uint8_t));
	
	struct pollfd *clients = calloc(stato->n, sizeof(struct pollfd));
	pollSetUp(stato, clients);


	for(;;)
	{
		//Wait for requests
		poll(clients, stato->n, -1);
		int fd = availFd(clients, stato->n, POLLIN);
		if(fd < 0)
		{
			LOGW("poll returned, but no file is ready for reading!\n");
			continue;	//This should never happen
		}

		//Read segment and identify train

		char buf[16] = {0};

		readWL(fd, buf, sizeof(buf));
		int tid = getTrainId(stato, fd);

		if(tid == -1)
		{
			LOGF("Il file descriptor %d non corrisponde a nessun treno!\n", fd);
			exit(EXIT_FAILURE);
		}

		//Chechk authorization

		struct posizione pos;
		map_getPosFromSeg(&pos, buf);

		if(!checkAuth(stato, map, &pos, tid, buf))
		{
			LOGW("Il treno %d ha tentato di accedere ad un segmento che non fa parte del suo itinerario o che non e la sua prossima tappa!\n", tid);
			writeWL(fd, "KO", sizeof("KO"));
			continue;
		}

		//Give permission

		if(pos.stazione)
		{
			writeWL(fd, "OK", sizeof("OK"));
		}
		else
		{
			if(segmenti[pos.stazione][pos.id] == 0)
			{
				writeWL(fd, "OK", sizeof("OK"));
			}
			else
			{
				writeWL(fd, "NO", sizeof("NO"));
				continue;
			}
		}

		//Check if the train moved

		readWL(fd, buf, sizeof(buf));

		if(strcmp(buf, "KO") == 0)
		{
			continue;
		}
		else if(strcmp(buf, "OK") != 0)
		{
			LOGW("Il treno %d ha inviato una risposta non valida! Ignoro e continuo.\n", tid);
			LOGD("%s\n", buf);
			continue;
		}
		
		updateStato(stato, &pos, tid);

	}

	free(segmenti[0]);
	free(segmenti[1]);
	free(clients);
	rc_freeMappa(map);

}

static int availFd(struct pollfd *clients, int numClients, short events)
{
	for(int i=0; i<numClients; i++)
	{
		if(clients[i].revents == events)
		{
			return clients[i].fd;
		}
	}

	return -1;
}

static inline void pollSetUp(struct stato_treni *stato, struct pollfd *pfd)
{
	for(int i=0; i<stato->n; i++)
	{
		pfd[i].fd = stato->fd[i];
		pfd[i].events = POLLIN;
	}
}

static inline int getTrainId(struct stato_treni *stato, int fd)
{
	for(int i=0; i<stato->n; i++)
	{
		if(stato->fd[i] == fd)
			return i;
	}

	return -1;
}

static bool checkAuth(struct stato_treni *stato, struct mappa *mappa, struct posizione *pos, uint8_t tid, const char *buf)
{
	LOGW("Unimplemented!\n");
	return true;
}

static inline void updateStato(struct stato_treni *stato, struct posizione *pos, uint8_t tid)
{
	memcpy(&stato->prevpos[tid], &stato->pos[tid], sizeof(struct posizione));
	memcpy(&stato->pos[tid], pos, sizeof(struct posizione));
}



