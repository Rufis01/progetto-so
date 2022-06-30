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
#include <signal.h>

#include "log.h"
#include "mappa.h"
#include "inline_rw_utils.h"
#include "socket.h"
#include "registro_client.h"

struct stato_treni
{
	int n;
	int *fd;
	bool *primaRichiesta;
	struct posizione *pos;
};

static bool _isRunning = true;

static int init(void);

static struct stato_treni *accettaConnessioni(int sfd);
static void servizio(struct stato_treni *stato, mappa_id mappa);

static int availFd(struct pollfd *clients, int numClients, short events);
static int remFd(struct pollfd *clients, int numClients, short events);
static void pollSetUp(struct stato_treni *stato, struct pollfd *pfd);
static int getTrainId(struct stato_treni *stato, int fd);
static bool checkAuth(struct stato_treni *stato, struct mappa *mappa, struct posizione *pos, uint8_t tid);
static void updateStato(struct stato_treni *stato, uint8_t *segmenti[2], struct posizione *pos, uint8_t tid);
static void freeStato(struct stato_treni* stato);

static void SIGUSR2_hanlder(int signum);

/* rbc richiede come parametro la mappa */
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

	int sfd = init();

	struct stato_treni *stato = accettaConnessioni(sfd);

	servizio(stato, map);
	
	close(sfd);

	freeStato(stato);

	rc_fini();
	log_fini();
}

static int init(void)
{
	struct sigaction act =
	{
		.sa_handler = SIGUSR2_hanlder
	} ;

	sigaction(SIGUSR2, &act, NULL);


	log_init("./log/rbc.log");
	while(!rc_init(false))
		sleep(1);
	
	int sfd = creaSocketUnix("./sock/rbc");

	pid_t pid = getpid();
	int fd = accept(sfd, NULL, NULL);	//padre_treni
	write(fd, &pid, sizeof(pid_t));
	close(fd);

	return sfd;
}

static struct stato_treni *accettaConnessioni(int sfd)
{
	int numTreni = rc_getNumeroTreni();

	struct stato_treni *stato = malloc(sizeof(struct stato_treni));
	stato->fd = calloc(numTreni, sizeof(int));
	stato->primaRichiesta = calloc(numTreni, sizeof(bool));
	stato->pos = calloc(numTreni, sizeof(struct posizione));
	stato->n = numTreni;

	for(int i=0; i<numTreni; i++)
	{
		int clientFd = accept(sfd, NULL, NULL);

		if(clientFd >= 0)
		{
			uint8_t trainId;
			readWL(clientFd, (char *)&trainId, sizeof(uint8_t));
			writeWL(clientFd, "OK", sizeof("OK"));
			stato->fd[trainId] = clientFd;
			stato->primaRichiesta[trainId] = true;
			LOGD("Il treno %d si e' connesso con file descriptor %d\n", trainId, clientFd);
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
	}

	return stato;
}

static void servizio(struct stato_treni *stato, mappa_id mappa)
{
	struct mappa *map = rc_getMappa();

	uint8_t numStazioni = map_getNumeroStazioni(map);
	uint8_t numBoe = map_getNumeroBoe(map);

	uint8_t *segmenti[2];
	
	segmenti[0] = calloc(numBoe, sizeof(uint8_t));
	segmenti[1] = calloc(numStazioni, sizeof(uint8_t));

	LOGD("Boe %d, Stazioni %d\n", numBoe, numStazioni);
	
	struct pollfd *clients = calloc(stato->n, sizeof(struct pollfd));
	pollSetUp(stato, clients);


	while(_isRunning)
	{
		//Wait for requests
		poll(clients, stato->n, -1);
		int fd = availFd(clients, stato->n, POLLIN);
		if(fd < 0)
		{
			if(remFd(clients, stato->n, POLLNVAL | POLLHUP | POLLERR) == stato->n)	//All trains terminated
			{
				sleep(1);
				continue;
			}
			LOGD("poll() e' ritornata, ma nessun file e' pronto alla lettura! Una socket e' stata chiusa?\n");
			continue;
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

		if(!checkAuth(stato, map, &pos, tid))
		{
			LOGW("Il treno %d ha tentato di accedere ad un segmento che non fa parte del suo itinerario o che non e la sua prossima tappa!\n", tid);
			writeWL(fd, "NO", sizeof("NO"));
			continue;
		}

		//Give permission (or not...)
		char tempo[32], cpos[5], npos[5];
		log_getCurrentTimeString(tempo);
		map_posStr(npos, &pos);
		if(stato->primaRichiesta[tid])
			snprintf(cpos, 5, "--");
		else
			map_posStr(cpos, &stato->pos[tid]);

		if(pos.stazione)
		{
			log_printf(LOG_INFO, "[%s] [Treno: T%d] [Attuale: %4s] [Richiesto: %4s] [Autorizzato: SI]\n", tempo, tid+1, cpos, npos);
			writeWL(fd, "OK", sizeof("OK"));
		}
		else
		{
			if(segmenti[pos.stazione][pos.id - 1] == 0)
			{
				log_printf(LOG_INFO, "[%s] [Treno: T%d] [Attuale: %4s] [Richiesto: %4s] [Autorizzato: SI]\n", tempo, tid+1, cpos, npos);
				writeWL(fd, "OK", sizeof("OK"));
			}
			else
			{
				log_printf(LOG_INFO, "[%s] [Treno: T%d] [Attuale: %4s] [Richiesto: %4s] [Autorizzato: NO]\n", tempo, tid+1, cpos, npos);
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
		
		updateStato(stato, segmenti, &pos, tid);

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

static int remFd(struct pollfd *clients, int numClients, short events)
{
	int closed = 0;
	for(int i=0; i<numClients; i++)
	{
		if(((clients[i].revents & events) != 0) && clients[i].fd >= 0)
			clients[i].fd = -clients[i].fd;
		if(clients[i].fd < 0)
			closed++;
	}

	return closed;
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

static bool checkAuth(struct stato_treni *stato, struct mappa *mappa, struct posizione *pos, uint8_t tid)
{
	struct itinerario *itin = &mappa->itinerari[tid];
	struct posizione *ppos = &stato->pos[tid];	//Posizione precedente

	struct posizione tpos;	//Posizione temporanea

	if(stato->primaRichiesta[tid])
	{
		map_getPosFromSeg(&tpos, itin->tappe[0]);
		return map_cmpPos(&tpos, pos);
	}

	for(int i = 0; i<itin->num_tappe - 1; i++)
	{
		map_getPosFromSeg(&tpos, itin->tappe[i]);
		if(map_cmpPos(&tpos, ppos))
		{
			map_getPosFromSeg(&tpos, itin->tappe[i + 1]);
			if(map_cmpPos(&tpos, pos) == true)
				return true;
			//altrimenti continua a cercare
		}
	}

	return false;
}

static inline void updateStato(struct stato_treni *stato, uint8_t *segmenti[2], struct posizione *pos, uint8_t tid)
{
	struct posizione *ppos = &stato->pos[tid];
	char segstr[5];
	if(stato->primaRichiesta[tid])
	{
		stato->primaRichiesta[tid] = false;
	}
	else
	{
		segmenti[ppos->stazione][ppos->id - 1]--;
		map_posStr(segstr, ppos);
		LOGD("Il treno %d libera il segmento %s; adesso ci sono %d treni\n", tid, segstr, segmenti[ppos->stazione][ppos->id - 1]);
	}
	segmenti[pos->stazione][pos->id - 1]++;
	map_posStr(segstr, pos);
	LOGD("Il treno %d occupa il segmento %s; adesso ci sono %d treni\n", tid, segstr, segmenti[pos->stazione][pos->id - 1]);
	memcpy(&stato->pos[tid], pos, sizeof(struct posizione));
}

static void freeStato(struct stato_treni* stato)
{
	free(stato->fd);
	free(stato->pos);
	free(stato->primaRichiesta);
	free(stato);
}

static void SIGUSR2_hanlder(int signum)
{
	if(signum != SIGUSR2)
	{
		LOGW("Il gestore di SIGUSR2 è stato invocato su un segnale diverso: %d\n", signum);
		return;
	}

	LOGD("Il gestore di SIGUSR2 è stato invocato!\n");

	_isRunning = false;
}
