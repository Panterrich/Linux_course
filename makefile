all: daemon
	
daemon:
	gcc src/daemon/daemon.c src/daemon/main.c -o demon -Wall