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

static void missione(modalita mod);
static bool occupaSegmento(const char* segmento);

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

	log_init("sciuli");
	missione(mod);
}

static void missione(modalita mod)
{
	rc_init(true);
	struct itinerario *itin = rc_getItinerario();
	bool okToMove, maConcessa = true;

	for(int i=0; i<itin->num_tappe; i++)
	{
		char *tappaCorrente = itin->tappe[i];
		//Prova ad occupare il segmento/stazione:
		//Se ETCS2 richiedi MA
		if(mod == ETCS2)
			maConcessa = rbc_richiediMA(tappaCorrente);

		if(!ISSTAZIONE(tappaCorrente) && maConcessa)
			okToMove = occupaSegmento(tappaCorrente);
		else
			okToMove = true;

		//Se ETCS2 comunica esito movimento
		if(mod == ETCS2)
			rbc_comunicaEsitoMovimento(maConcessa == true && okToMove == true);
		
		//Dormi 2 secondi
		sleep(2);
	}

	rc_freeItinerario(itin);
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

	if(snprintf(filepath, sizeof(filepath), "./segmenti/%s", segmento) >= sizeof(filepath))
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

	//Rilascia il lock (anche se dovrebbe venir rilasciato sulla close)
	LOGD("Controllare che close rilascia il lock!\n");
	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLKW, &lock);

	close(fd);

	return segmentoOccupato;
}