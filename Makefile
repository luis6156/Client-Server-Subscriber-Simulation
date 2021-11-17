CCX = g++

CFLAGS = -std=c++11 -Wall -Wextra -Wno-unused-value

.PHONY: clean run_server run_client

all: build 

build: server subscriber

server: server.o server_helper.o
	$(CCX) $(CFLAGS) $^ -o $@

server.o: server.cpp
	$(CCX) $(CFLAGS) $^ -c

server_helper.o: server_helper.cpp
	$(CCX) $(CFLAGS) $^ -c

subscriber: subscriber.o subscriber_helper.o
	$(CCX) $(CFLAGS) $^ -o $@

subscriber.o: subscriber.cpp
	$(CCX) $(CFLAGS) $^ -c

subscriber_helper.o: subscriber_helper.cpp
	$(CCX) $(CFLAGS) $^ -c

clean:
	rm -f *.o
	rm -f server subscriber
