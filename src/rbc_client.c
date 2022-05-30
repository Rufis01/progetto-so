
#include "log.h"
#include "rbc_client.h"

bool rbc_init(void)
{
	LOGD("called\n");
	return true;
}

bool rbc_richiediMA(char *segmento)
{
	LOGD("called with arg segmento: %s\n", segmento);
	return true;
}

void rbc_comunicaEsitoMovimento(bool esito)
{
	LOGD("called with arg esito: %d\n", esito);
}

void rbc_fini(void)
{
	LOGD("called\n");
}