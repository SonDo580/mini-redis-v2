CXX      := g++
CXXFLAGS := -Wall -Wextra -Og -g

all: server client

server: server.cpp common.cpp common.hpp server_utils.cpp server_utils.hpp hashtable.cpp hashtable.hpp zset.cpp zset.hpp avl.cpp avl.hpp 
	$(CXX) $(CXXFLAGS) server.cpp common.cpp server_utils.cpp hashtable.cpp zset.cpp avl.cpp -o server.exe

client: client.cpp common.cpp common.hpp
	$(CXX) $(CXXFLAGS) client.cpp common.cpp -o client.exe

test_avl: test_avl.cpp avl.cpp avl.hpp
	$(CXX) $(CXXFLAGS) test_avl.cpp avl.cpp -o test_avl.exe
test_offset: test_offset.cpp avl.cpp avl.hpp
	$(CXX) $(CXXFLAGS) test_offset.cpp avl.cpp -o test_offset.exe

.PHONY: clean
clean:
	rm -f *.exe