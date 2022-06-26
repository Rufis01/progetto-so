#CPP stands fo C PreProcessor
CPPFLAGS := -I./include -DDEBUG
CFLAGS := -Wall -Wpedantic

SRC := ./src
OBJ := ./obj
BIN := ./bin
INC := ./include

all : dir padre_treni processo_treno registro rbc

zip : 
	tar -czf progetto.tar.gz makefile include src .vscode

clean : 
	rm -rf bin
	rm -rf obj
	rm -f progetto.tar.gz

dir : 
	mkdir -p obj
	mkdir -p bin

rbc : $(SRC)/rbc.c\
      $(INC)/registro_client.h $(INC)/socket.h $(INC)/mappa.h $(INC)/log.h\
      dir registro_client.o socket.o mappa.o log.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC)/rbc.c $(OBJ)/registro_client.o $(OBJ)/socket.o $(OBJ)/mappa.o $(OBJ)/log.o -o $(BIN)/$@

registro : $(SRC)/registro.c\
           $(INC)/mappa.h $(INC)/socket.h $(INC)/log.h $(INC)/inline_rw_utils.h\
           dir socket.o mappa.o log.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC)/registro.c $(OBJ)/socket.o $(OBJ)/mappa.o $(OBJ)/log.o -o $(BIN)/$@

padre_treni : $(SRC)/padre_treni.c\
              $(INC)/registro_client.h $(INC)/mappa.h $(INC)/socket.h $(INC)/log.h\
              dir registro_client.o socket.o mappa.o log.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC)/padre_treni.c $(OBJ)/registro_client.o $(OBJ)/socket.o $(OBJ)/mappa.o $(OBJ)/log.o -o $(BIN)/$@

processo_treno : $(SRC)/processo_treno.c\
                 $(INC)/modalita.h $(INC)/mappa.h\
                 dir registro_client.o rbc_client.o socket.o mappa.o log.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC)/processo_treno.c $(OBJ)/registro_client.o $(OBJ)/rbc_client.o $(OBJ)/socket.o $(OBJ)/mappa.o $(OBJ)/log.o -o $(BIN)/$@

registro_client.o : $(SRC)/registro_client.c\
                    $(INC)/registro_client.h $(INC)/socket.h $(INC)/mappa.h $(INC)/inline_rw_utils.h $(INC)/log.h\
                    dir 
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(SRC)/registro_client.c -o $(OBJ)/$@

rbc_client.o : $(SRC)/rbc_client.c\
               $(INC)/rbc_client.h $(INC)/socket.h $(INC)/log.h\
               dir 
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(SRC)/rbc_client.c -o $(OBJ)/$@

socket.o : $(SRC)/socket.c\
           $(INC)/socket.h $(INC)/log.h\
           dir 
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(SRC)/socket.c -o $(OBJ)/$@

log.o : $(SRC)/log.c\
        $(INC)/log.h\
        dir 
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(SRC)/log.c -o $(OBJ)/$@

mappa.o : $(SRC)/mappa.c\
          $(INC)/mappa.h\
          dir 
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(SRC)/mappa.c -o $(OBJ)/$@

.PHONY: all dir zip clean
