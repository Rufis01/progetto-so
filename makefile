CPPFLAGS := -I./include -DDEBUG#CPP stands for C PreProcessor
CFLAGS := -Wall -Wpedantic

SRC := ./src
OBJ := ./obj
BIN := ./bin
INC := ./include

all : padre_treni processo_treno registro

registro : $(SRC)/processo_registro.c\
           $(INC)/mappa.h $(INC)/log.h $(INC)/inline_rw_utils.h\
           mappa.o log.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC)/processo_registro.c $(OBJ)/mappa.o $(OBJ)/log.o -o $(BIN)/$@

padre_treni : $(SRC)/padre_treni.c\
              $(INC)/registro_client.h $(INC)/mappa.h $(INC)/log.h\
              registro_client.o mappa.o log.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC)/padre_treni.c $(OBJ)/registro_client.o $(OBJ)/mappa.o $(OBJ)/log.o -o $(BIN)/$@

processo_treno : $(SRC)/processo_treno.c\
                 $(INC)/modalita.h $(INC)/mappa.h\
                 registro_client.o rbc_client.o mappa.o log.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC)/processo_treno.c $(OBJ)/registro_client.o $(OBJ)/rbc_client.o $(OBJ)/mappa.o $(OBJ)/log.o -o $(BIN)/$@

registro_client.o : $(SRC)/registro_client.c\
                    $(INC)/registro_client.h $(INC)/mappa.h $(INC)/inline_rw_utils.h $(INC)/log.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(SRC)/registro_client.c -o $(OBJ)/$@

rbc_client.o : $(SRC)/rbc_client.c\
               $(INC)/rbc_client.h $(INC)/log.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(SRC)/rbc_client.c -o $(OBJ)/$@

log.o : $(SRC)/log.c\
        $(INC)/log.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(SRC)/log.c -o $(OBJ)/$@

mappa.o : $(SRC)/mappa.c\
          $(INC)/mappa.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(SRC)/mappa.c -o $(OBJ)/$@

.PHONY: all
