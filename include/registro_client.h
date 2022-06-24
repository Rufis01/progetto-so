#ifndef REGISTRO_CLIENT_H
#define REGISTRO_CLIENT_H

#include <stdbool.h>

#include <mappa.h>

bool rc_init(bool isTrain);
struct itinerario *rc_getItinerario(void);	//TRAIN
struct mappa *rc_getMappa(void);			//SUPER
int rc_getNumeroTreni(void);				//SUPER
void rc_freeItinerario(struct itinerario *itin);
void rc_freeMappa(struct mappa *mappa);
void rc_fini(void);

#endif
