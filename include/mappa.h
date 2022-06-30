#ifndef MAPPA_H
#define MAPPA_H

#include <stdbool.h>
#include <stdint.h>

#define NUM_SEGMENTI 16

#define ISSTAZIONE(seg) (seg[0] == 'S')

struct posizione
{
	bool stazione;
	uint8_t id;
};

typedef enum
{
	MAPPA_NON_VALIDA = -1,
	MAPPA1,
	MAPPA2
} mappa_id;

struct itinerario
{
	uint8_t num_itinerario;
	uint8_t num_tappe;
	char **tappe;
};

struct mappa
{
	uint8_t treni;
	struct itinerario *itinerari;
};

mappa_id m_getMappaId(const char *nome);
uint8_t map_getNumeroStazioni(struct mappa *mappa);
uint8_t map_getNumeroBoe(struct mappa *mappa);
bool map_getPosFromSeg(struct posizione *pos, const char *buf);
bool map_cmpPos(struct posizione *pos1, struct posizione *pos2);
void map_posStr(char out[5], struct posizione *pos);

#endif
