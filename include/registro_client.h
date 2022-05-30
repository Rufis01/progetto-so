#ifndef REGISTRO_CLIENT_H
#define REGISTRO_CLIENT_H

#include <stdbool.h>

bool rc_init(bool isTrain);
struct itinerario *rc_getItinerario(void);	//TRAIN
struct itinerario **rc_getMappa(void);		//SUPER
int rc_getNumeroTreni(void);				//SUPER
void rc_freeItinerario(struct itinerario *itin);
void rc_freeMappa(struct itinerario **itin);
void rc_fini(void);

#endif