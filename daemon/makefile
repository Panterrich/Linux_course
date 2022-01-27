all: daemon controller
	
.PHONY: daemon controller

daemon:
	gcc src/daemon/daemon.c src/daemon/backup.c src/daemon/main.c 					\
		src/hashtable/hashtable.c src/hashtable/list_pointer.c -o demon -Wall

controller:
	gcc src/controller/main.c src/controller/controller.c -o controller -Wall