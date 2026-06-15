CXX      := g++
CXXFLAGS := -Wall -Wextra -Og -g

all: server client

server: server.cpp common.cpp hashtable.cpp common.hpp hashtable.hpp
	$(CXX) $(CXXFLAGS) server.cpp common.cpp hashtable.cpp -o server.exe

client: client.cpp common.cpp common.hpp
	$(CXX) $(CXXFLAGS) client.cpp common.cpp -o client.exe

.PHONY: clean
clean:
	rm -f *.exe