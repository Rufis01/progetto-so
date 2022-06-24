#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "mappa.h"
#include "registro_client.h"
#include "log.h"
#include "socket.h"

///TODO: unlink sockets
///TODO: usoErrato ok??
///TODO: rivedere ordine di controllo. Controllare subito che mappa sia ok?
///TODO: kill registro

static void usoErrato(void);
static void creaBoe(void);
static void ETCS1(char* mappa);
static void ETCS2(char* mappa);
static void ETCS2_RBC(char *mappa);

static void spawnRegistro(char *mappa);
static pid_t getRBCpid(void);

static void aspettaTreni(int numero);

int main(int argc, char **argv)
{
	if(argc <= 2)
		usoErrato();
		//EXITS
	mkdir("./log", 0777);

	log_setLogLevel(LOG_DEBUG);
	log_init((argc <= 3) ? "./log/padre_treni.log" : "./log/padre_rbc.log");
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
	umask(0777-0666);

	for(int i = 0; i < NUM_SEGMENTI; i++)
	{
		if(snprintf(segpath, sizeof(segpath), "./boe/MA%d", i+1) >= sizeof(segpath))
		{
			LOGF("La lungezza del path del file di segmento eccede la lunghezza massima!\n");
			exit(EXIT_FAILURE);
		}

		int fd = open(segpath, O_CREAT | O_WRONLY, 0666);
		write(fd, "0", 1);
		close(fd);
	}

	umask(0777-0777);
}

static void ETCS1(char *mappa)
{
	mappa_id map = m_getMappaId(mappa);
	if(map == MAPPA_NON_VALIDA)
		usoErrato();
		//EXITS
	spawnRegistro(mappa);

	while(!rc_init(false))
		sleep(1);

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
	aspettaTreni(treni);
}

///TODO: merge ETCS1 and ETCS2?

static void ETCS2(char *mappa)
{
	mappa_id map = m_getMappaId(mappa);
	if(map == MAPPA_NON_VALIDA)
		usoErrato();
		//EXITS
	

	spawnRegistro(mappa);

	pid_t rbcpid = getRBCpid();
	
	while(!rc_init(false))
		sleep(1);

	int treni = rc_getNumeroTreni();
	rc_fini();

	for(int i=0; i<treni; i++)
	{
		if(fork() == 0)		//Sono il figlio
		{
			execl("./processo_treno", "processo_treno", "ETCS2", (char *)NULL);
			exit(EXIT_SUCCESS);
		}
	}

	aspettaTreni(treni);
	kill(rbcpid, SIGUSR2);
}

static void ETCS2_RBC(char *mappa)
{
	mappa_id map = m_getMappaId(mappa);
	if(map == MAPPA_NON_VALIDA)
		usoErrato();
		//EXITS

	execl("./rbc", "rbc", mappa, (char *)NULL);
	exit(EXIT_FAILURE);
}

static void spawnRegistro(char *mappa)
{
	if(fork() == 0)
	{
		execl("./registro", "registro", mappa, (char *)NULL);
		exit(EXIT_SUCCESS);
	}
}

static pid_t getRBCpid(void)
{
	int sfd;
	pid_t pid;
	do
	{
		sfd = connettiSocketUnix("./sock/rbc");
		sleep(1);
	} while (sfd < 0);
	
	LOGD("RBC socket fd is %d\n", sfd);
	read(sfd, &pid, sizeof(pid_t));
	close(sfd);
	LOGD("RBC PID is %d\n", pid);
	return pid;
}


static void usoErrato(void)
{
	puts("sciuli!");
	exit(EXIT_FAILURE);
}

static void aspettaTreni(int numero)
{
	sigset_t sigmask;
	siginfo_t info;
	int *pid = calloc(numero, sizeof(int));

	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGUSR1);

	sigprocmask(SIG_BLOCK, &sigmask, NULL);

	for(int i=0; i<numero; i++)
	{
		sigwaitinfo(&sigmask, &info);
		kill(info.si_pid, SIGUSR1);
		pid[i] = info.si_pid;
		LOGD("PID %d ha terminato\n", info.si_pid);
	} 	

	sleep(1);

	for(int i=0; i<numero; i++)
		kill(pid[i], SIGKILL);

}
