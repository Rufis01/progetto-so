#include <stdint.h>
#include <string.h>

#include "rbc_client.h"

#include "log.h"
#include "socket.h"
#include "inline_rw_utils.h"

static int _fd;

bool rbc_init(uint8_t tid)
{
	_fd = connettiSocketUnix("./sock/rbc");
	if(_fd < 0)
		return false;

	char buf[8] = {0};

	writeWL(_fd, (char *)&tid, sizeof(tid));
	readWL(_fd, buf, sizeof(buf));

	if(strcmp(buf, "OK") == 0)
	{
		LOGD("Connesso!\n");
		return true;
	}
	
	return false;
}

bool rbc_richiediMA(const char *segmento)
{
	char buf[8] = {0};

	writeWL(_fd, segmento, strlen(segmento));
	readWL(_fd, buf, sizeof(buf));

	if(strcmp(buf, "OK") == 0)
	{
		LOGD("MA concessa!\n");
		return true;
	}

	LOGD("MA non concessa!\n");
	
	return false;
}

void rbc_comunicaEsitoMovimento(bool esito)
{
	if(esito == true)
		writeWL(_fd, "OK", sizeof("OK"));
	else
		writeWL(_fd, "KO", sizeof("KO"));
}

void rbc_fini(void)
{
	close(_fd);
}
