#include <string.h>

#include "mappa.h"

mappa_id m_getMappaId(const char *nome)
{
	if(!nome)
		return MAPPA_NON_VALIDA;
		
	if(strcmp(nome, "MAPPA1") == 0)
		return MAPPA1;
	if(strcmp(nome, "MAPPA2") == 0)
		return MAPPA2;

	return MAPPA_NON_VALIDA;
}
