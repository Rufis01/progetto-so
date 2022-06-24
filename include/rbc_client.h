#ifndef RBC_CLIENT_H
#define RBC_CLIENT_H

#include <stdbool.h>

bool rbc_init(uint8_t tid);
bool rbc_richiediMA(const char *segmento);
void rbc_comunicaEsitoMovimento(bool esito);
void rbc_fini(void);

#endif
