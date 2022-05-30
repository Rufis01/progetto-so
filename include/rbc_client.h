#ifndef RBC_CLIENT_H
#define RBC_CLIENT_H

#include <stdbool.h>

bool rbc_init(void);
bool rbc_richiediMA(char *segmento);
void rbc_comunicaEsitoMovimento(bool esito);
void rbc_fini(void);

#endif