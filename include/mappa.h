#ifndef MAPPA_H
#define MAPPA_H

#include <stdint.h>

#define NUM_SEGMENTI 16

#define ISSTAZIONE(seg) (seg[0] == 'S')

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
struct mappa *m_caricaDaFile(const char *path);
void m_freeMappa(struct mappa *mappa);

#endif
