#include <unistd.h>
#include <stdlib.h>

#include "mappa.h"
#include "registro_client.h"

///TODO: unlink sockets
///TODO: usoErrato ok??
static void usoErrato(void);
static void creaBoe(void);
static void ETCS1(char* mappa);
static void ETCS2(char* mappa);

int main(int argc, char **argv)
{
	if(argc <= 2)
		usoErrato();
		//EXITS

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

	return 0;
}

static void ETCS1(char *mappa)
{
	mappa_id map = m_getMappaId(mappa);
	if(map == MAPPA_NON_VALIDA)
		usoErrato();
		//EXITS
	
	int treni = rc_getNumeroTreni();

	for(int i=0; i<treni; i++)
	{
		if(fork() == 0)		//Sono il figlio
			exit(0);//execve();
	}

	for(int i=0; i<treni; i++)
		if(!wait())
		{
			perror("padre_treni:");
		}
}

static void ETCS2(char *mappa)
{
	mappa_id map = m_getMappaId(mappa);
	if(map == MAPPA_NON_VALIDA)
		usoErrato();
		//EXITS
	
	int treni = rc_getNumeroTreni();

	for(int i=0; i<treni; i++)
	{
		if(fork() == 0)		//Sono il figlio
			exit(0);//execve();
	}

	for(int i=0; i<treni; i++)
		if(!wait())
		{
			perror("padre_treni:");
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


static void usoErrato(void)
{
	puts("");
	exit(EXIT_FAILURE);
}
