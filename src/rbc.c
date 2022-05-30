#include <stdlib.h>
#include <unistd.h>

#include "mappa.h"

/* rbc richiede come parametro la mappa */
int main(int argc, char **argv)
{
	if(argc != 2)
	{
		exit(EXIT_FAILURE);
	}

	mappa_id map = m_getMappaId(argv[2]);

	if(map == MAPPA_NON_VALIDA)
	{
		exit(EXIT_FAILURE);
	}

}
