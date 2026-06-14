CXX      := g++
CXXFLAGS := -Wall -Wextra -O2 -g

all: server client

server:
	$(CXX) $(CXXFLAGS) server.cpp common.cpp hashtable.cpp -o server.exe

client:
	$(CXX) $(CXXFLAGS) client.cpp common.cpp -o client.exe

.PHONY: clean
clean:
	rm -f *.exe