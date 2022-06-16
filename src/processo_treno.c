#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include "modalita.h"
#include "mappa.h"
#include "registro_client.h"
#include "rbc_client.h"
#include "log.h"

#define MAX_RETRIES 5

static void init(modalita mod);
static void missione(modalita mod, struct itinerario *itin);
static bool occupaSegmento(const char* segmento);
static void liberaSegmento(const char* segmento);

/* processo_treno richiede come parametro la modalità (ETCS1/2)*/
int main(int argc, char **argv)
{
	modalita mod;

	if(argc != 2)
	{
		exit(EXIT_FAILURE);
	}

	if(strcmp(argv[1], "ETCS1") == 0)
		mod = ETCS1;
	else if(strcmp(argv[1], "ETCS2") == 0)
		mod = ETCS2;
	else
	{
		exit(EXIT_FAILURE);
	}

	init(mod);
}

static void init(modalita mod)
{
	//We don't want half a K sitting on the stack
	//Let's allocate it on the heap and free it when we're done
	char *logpath_pid = calloc(256,1);
	char *logpath = calloc(256,1);

	if(snprintf(logpath_pid, 255, "./log/pid_%d.log", getpid()) >= 255)
	{
		LOGE("La lungezza del path del file di segmento eccede la lunghezza massima!\n");
		exit(EXIT_FAILURE);
	}
	log_init(logpath_pid);

	struct itinerario *itin;
	rc_init(true);
	itin = rc_getItinerario();

	if(snprintf(logpath, 255, "./log/treno%d.log", itin->num_itinerario) >= 255)
	{
		LOGE("La lungezza del path del file di segmento eccede la lunghezza massima!\n");
		exit(EXIT_FAILURE);
	}

	log_rename(logpath_pid, logpath);
	free(logpath_pid);
	free(logpath);

	missione(mod, itin);

	rc_freeItinerario(itin);
	log_fini();
}

static void missione(modalita mod, struct itinerario *itin)
{
	bool okToMove, maConcessa = true;
	int fail = 0;

	for(int i=0; i<itin->num_tappe; i++)
	{
		if(fail >= MAX_RETRIES)
		{
			LOGW("The train could not move. Aborting!\n");
			exit(EXIT_FAILURE);
		}

		char *tappaCorrente = itin->tappe[i];
		LOGI("%s %d\n", tappaCorrente, time(NULL));
		//Prova ad occupare il segmento/stazione:
		//Se ETCS2 richiedi MA
		if(mod == ETCS2)
			maConcessa = rbc_richiediMA(tappaCorrente);

		if(!ISSTAZIONE(tappaCorrente) && maConcessa)
			okToMove = occupaSegmento(tappaCorrente);
		else
			okToMove = true;


		if(maConcessa == true && okToMove == true)
		{
			fail = 0;
			if(i>0)
			{
				char *tappaPrecedente = itin->tappe[i-1];
				liberaSegmento(tappaPrecedente);
			}
			//Se ETCS2 comunica esito movimento
			if(mod == ETCS2)
				rbc_comunicaEsitoMovimento(true);
		}
		else
		{
			i--;
			fail++;
			//Se ETCS2 comunica esito movimento
			if(mod == ETCS2)
				rbc_comunicaEsitoMovimento(false);
		}

		//Dormi 2 secondi
		sleep(2);
	}
}

static bool occupaSegmento(const char* segmento)
{
	int fd;
	char filepath[256], segmentoOccupato;
	struct flock lock =
	{
		.l_type   = F_WRLCK,	/* Type of lock: F_RDLCK, F_WRLCK, F_UNLCK */
		.l_whence = SEEK_SET,	/* How to interpret l_start: SEEK_SET, SEEK_CUR, SEEK_END */
		.l_start  = 0,   		/* Starting offset for lock */
		.l_len    = 1,     		/* Number of bytes to lock */
	};

	if(!segmento)
	{
		LOGE("Argomento non valido!\n");
		exit(EXIT_FAILURE);
	}

	if(snprintf(filepath, sizeof(filepath), "./boe/%s", segmento) >= sizeof(filepath))
	{
		LOGE("La lungezza del path del file di segmento eccede la lunghezza massima!\n");
		exit(EXIT_FAILURE);
	}

	fd = open(filepath, O_RDWR);

	if(fd < 0)
	{
		LOGE("Impossibile aprire il file!\n");
		exit(EXIT_FAILURE);
	}

	//Ottieni il lock
	if(fcntl(fd, F_SETLKW, &lock) < 0)
	{
		LOGE("Impossibile aquisire il lock!\n");
		exit(EXIT_FAILURE);
	}

	//Controlla che il segmento sia vuoto
	read(fd, &segmentoOccupato, 1);
	lseek(fd, 0, SEEK_SET);

	//Se è vuoto occupalo
	if(segmentoOccupato == '0')
		write(fd, "1", 1);

	//Rilascia il lock
	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLKW, &lock);

	close(fd);

	return segmentoOccupato == '0';
}

static void liberaSegmento(const char* segmento)
{
	int fd;
	char filepath[256], segmentoOccupato;

	if(!segmento)
	{
		LOGE("Argomento non valido!\n");
		exit(EXIT_FAILURE);
	}

	if(ISSTAZIONE(segmento))
		return;

	if(snprintf(filepath, sizeof(filepath), "./boe/%s", segmento) >= sizeof(filepath))
	{
		LOGE("La lungezza del path del file di segmento eccede la lunghezza massima!\n");
		exit(EXIT_FAILURE);
	}

	fd = open(filepath, O_RDWR);

	if(fd < 0)
	{
		LOGE("Impossibile aprire il file!\n");
		exit(EXIT_FAILURE);
	}

	write(fd, "0", 1);

	close(fd);
}