CXX      := g++
CXXFLAGS := -Wall -Wextra -Og -g

all: server client

server: server.cpp common.cpp common.hpp hashtable.cpp hashtable.hpp
	$(CXX) $(CXXFLAGS) server.cpp common.cpp hashtable.cpp -o server.exe

client: client.cpp common.cpp common.hpp
	$(CXX) $(CXXFLAGS) client.cpp common.cpp -o client.exe

test_avl: test_avl.cpp avl.cpp avl.hpp common.cpp common.hpp
	$(CXX) $(CXXFLAGS) test_avl.cpp avl.cpp common.cpp -o test_avl.exe

.PHONY: clean
clean:
	rm -f *.exe