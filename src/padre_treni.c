#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "mappa.h"
#include "registro_client.h"
#include "log.h"

///TODO: unlink sockets
///TODO: usoErrato ok??
///TODO: rivedere ordine di controllo. Controllare subito che mappa sia ok?

static void usoErrato(void);
static void creaBoe(void);
static void ETCS1(char* mappa);
static void ETCS2(char* mappa);
static void ETCS2_RBC(char *mappa);

static void spawnRegistro(char *mappa);

int main(int argc, char **argv)
{
	if(argc <= 2)
		usoErrato();
		//EXITS
	log_setLogLevel(LOG_INFO);
	log_init("./log/padre_treni.log");
	creaBoe();

	if(strcmp(argv[1], "ETCS1") == 0)
	{
		ETCS1(argv[2]);
	}
	else if(strcmp(argv[1], "ETCS2") == 0)
	{
		if(argc <= 3)
			ETCS2(argv[2]);
		else if(strcmp(argv[2], "RBC") == 0)
			ETCS2_RBC(argv[3]);
		else
			usoErrato();
			//EXITS
	}

	log_fini();
	return 0;
}

static void creaBoe(void)
{
	char segpath[32] = {0};

	mkdir("./boe", 0777);
	//umask(0666);

	for(int i = 0; i < NUM_SEGMENTI; i++)
	{
		if(snprintf(segpath, sizeof(segpath), "./boe/MA%d", i+1) >= sizeof(segpath))
		{
			LOGE("La lungezza del path del file di segmento eccede la lunghezza massima!\n");
			exit(EXIT_FAILURE);
		}

		int fd = open(segpath, O_CREAT | O_WRONLY, 0666);
		write(fd, "0", 1);
		close(fd);
	}

	//umask(0777);
}

static void ETCS1(char *mappa)
{
	mappa_id map = m_getMappaId(mappa);
	if(map == MAPPA_NON_VALIDA)
		usoErrato();
		//EXITS
	spawnRegistro(mappa);

	rc_init(false);
	int treni = rc_getNumeroTreni();
	rc_fini();

	for(int i=0; i<treni; i++)
	{
		if(fork() == 0)		//Sono il figlio
		{
			execl("./processo_treno", "processo_treno", "ETCS1", (char *)NULL);
			exit(EXIT_SUCCESS);
		}
	}

	for(int i=0; i<treni; i++)
	{
		int status;
		if(!wait(&status))
		{
			perror("padre_treni:");
		}
	}
}

static void ETCS2(char *mappa)
{
	mappa_id map = m_getMappaId(mappa);
	if(map == MAPPA_NON_VALIDA)
		usoErrato();
		//EXITS
	spawnRegistro(mappa);
	
	int treni = rc_getNumeroTreni();

	for(int i=0; i<treni; i++)
	{
		if(fork() == 0)		//Sono il figlio
			exit(0);//execve();
	}

	for(int i=0; i<treni; i++)
	{
		int status;
		if(!wait(&status))
		{
			perror("padre_treni:");
		}
	}
}

static void ETCS2_RBC(char *mappa)
{
	mappa_id map = m_getMappaId(mappa);
	if(map == MAPPA_NON_VALIDA)
		usoErrato();
		//EXITS

	if(fork() == 0)		//Sono il figlio
		exit(0);//execve();
}

static void spawnRegistro(char *mappa)
{
	if(fork() == 0)
	{
		execl("./registro", "registro", mappa, (char *)NULL);
		exit(EXIT_SUCCESS);
	}

	sleep(2);
}


static void usoErrato(void)
{
	puts("");
	exit(EXIT_FAILURE);
}
