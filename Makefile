CC=cc
CFLAGS=-W -Wall -O2
BIN=alidval
OBJ=alidval.o

all: $(BIN)

clean:
	rm -f $(BIN) $(OBJ)

$(BIN): $(OBJ)
	${CC} $(OBJ) -o $(BIN)

alidval.o: alidval.c
	${CC} $(CFLAGS) -c alidval.c -o alidval.o

# EOF
