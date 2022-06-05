CPPFLAGS := -I./include #CPP stands for C PreProcessor

SRC := ./src
OBJ := ./obj
BIN := ./bin
INC := ./include

all : padre_treni processo_treno

padre_treni : $(SRC)/padre_treni.c\
              $(INC)/registro_client.h $(INC)/mappa.h $(INC)/log.h\
              registro_client.o mappa.o log.o
	$(CC) $(CPPFLAGS) $(SRC)/padre_treni.c $(OBJ)/registro_client.o $(OBJ)/mappa.o $(OBJ)/log.o -o $(BIN)/$@

processo_treno : $(SRC)/processo_treno.c\
                 $(INC)/modalita.h $(INC)/mappa.h\
		 registro_client.o rbc_client.o mappa.o log.o
	$(CC) $(CPPFLAGS) $(SRC)/processo_treno.c $(OBJ)/registro_client.o $(OBJ)/rbc_client.o $(OBJ)/mappa.o $(OBJ)/log.o -o $(BIN)/$@

registro_client.o : $(SRC)/registro_client.c\
                    $(INC)/registro_client.h $(INC)/mappa.h $(INC)/inline_rw_utils.h $(INC)/log.h
	$(CC) $(CPPFLAGS) -c $(SRC)/registro_client.c -o $(OBJ)/$@

rbc_client.o : $(SRC)/rbc_client.c\
               $(INC)/rbc_client.h $(INC)/log.h
	$(CC) $(CPPFLAGS) -c $(SRC)/rbc_client.c -o $(OBJ)/$@

log.o : $(SRC)/log.c\
        $(INC)/log.h
	$(CC) $(CPPFLAGS) -c $(SRC)/log.c -o $(OBJ)/$@

mappa.o : $(SRC)/mappa.c\
          $(INC)/mappa.h
	$(CC) $(CPPFLAGS) -c $(SRC)/mappa.c -o $(OBJ)/$@

.PHONY: all
