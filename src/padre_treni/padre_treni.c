#include <unistd.h>
#include <stdlib.h>

#include "padre_treni.h"

void usoErrato(void)
{
	puts("");
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	if(argc <= 2)
		usoErrato();

	creaBoe();

	if(strcmp(argv[1], "ETCS1") == 0)
		ETCS1(getMappaId(argv[2]));
	else if(strcmp(argv[1], "ETCS2") == 0)
	{
		if(argc <= 3)
			ETCS2(getMappaId(argv[2]));
		else if(strcmp(argv[2], "RBC") == 0)
			ETCS2_RBC(getMappaId(argv[3]));
		else
			usoErrato();
	}

	return 0;
}
