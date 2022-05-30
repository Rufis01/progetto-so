#ifndef MAPPA_H
#define MAPPA_H

#define ISSTAZIONE(seg) (seg[0] == 'S')

typedef enum
{
	MAPPA_NON_VALIDA = -1,
	MAPPA1,
	MAPPA2
} mappa_id;

struct itinerario
{
	unsigned short num_itinerario;
	unsigned short num_tappe;
	char **tappe;
};

struct mappa
{
	unsigned short treni;
	struct itinerario *itinerari;
};

mappa_id m_getMappaId(const char *nome);

#endif
