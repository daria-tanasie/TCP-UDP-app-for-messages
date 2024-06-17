CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

all: server subscriber

server: server.c

subscrber: subscrber.c

.PHONY: clean run_server run_subscrber

run_server:
	./server ${IP_SERVER} ${PORT_SERVER}

run_subscrber:
	./subscrber ${ID_CLIENT} ${IP_SERVER} ${PORT_SERVER}

clean:
	rm -rf server subscriber *.o *.dSYM
