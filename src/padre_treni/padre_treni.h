#ifndef PADRE_TRENI_H
#define PADRE_TRENI_H

typedef enum mappa
{
	MAPPA1,
	MAPPA2
} mappa_t;

void usoErrato(void);
mappa_t getMappaId(const char *nome);
void creaBoe(void);
void ETCS1(mappa_t mappa);
void ETCS2(mappa_t mappa);

#endif