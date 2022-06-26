#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "mappa.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))

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

uint8_t map_getNumeroStazioni(struct mappa *mappa)
{
	struct posizione pos;
	uint8_t stazioni = 0;
	for(int i = 0; i<mappa->treni; i++)
	{
		int numTappe = mappa->itinerari[i].num_tappe;
		char **tappe = mappa->itinerari[i].tappe;
		for(int j=0; j<numTappe; j++)
		{
			map_getPosFromSeg(&pos, tappe[j]);
			if(pos.stazione)
				stazioni = MAX(stazioni, pos.id);
		}
	}

	return stazioni;
}

uint8_t map_getNumeroBoe(struct mappa *mappa)
{
	struct posizione pos;
	uint8_t boe = 0;
	
	for(int i = 0; i<mappa->treni; i++)
	{
		int numTappe = mappa->itinerari[i].num_tappe;
		char **tappe = mappa->itinerari[i].tappe;
		for(int j=0; j<numTappe; j++)
		{
			map_getPosFromSeg(&pos, tappe[j]);
			if(!pos.stazione)
				boe = MAX(boe, pos.id);
		}
	}

	return boe;
}

bool map_getPosFromSeg(struct posizione *pos, const char *buf)
{

	pos->stazione = ISSTAZIONE(buf);
	pos->id = atoi(buf + (pos->stazione ? 1 : 2));

	return pos->id != 0;
}

bool map_cmpPos(struct posizione *pos1, struct posizione *pos2)
{
	return pos1->id == pos2->id && pos1->stazione == pos2->stazione;
}
