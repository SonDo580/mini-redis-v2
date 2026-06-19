CXX      := g++
CXXFLAGS := -Wall -Wextra -O0 -g

all: server client

server: server.cpp common.cpp common.hpp server_utils.cpp server_utils.hpp state.cpp state.hpp exec.cpp exec.hpp hashtable.cpp hashtable.hpp zset.cpp zset.hpp avl.cpp avl.hpp list.cpp list.hpp heap.cpp heap.hpp
	$(CXX) $(CXXFLAGS) server.cpp common.cpp server_utils.cpp state.cpp exec.cpp hashtable.cpp zset.cpp avl.cpp list.cpp heap.cpp -o server.exe

client: client.cpp common.cpp common.hpp
	$(CXX) $(CXXFLAGS) client.cpp common.cpp -o client.exe

test_avl: test_avl.cpp avl.cpp avl.hpp
	$(CXX) $(CXXFLAGS) test_avl.cpp avl.cpp -o test_avl.exe
test_offset: test_offset.cpp avl.cpp avl.hpp
	$(CXX) $(CXXFLAGS) test_offset.cpp avl.cpp -o test_offset.exe
test_heap: test_heap.cpp heap.cpp heap.hpp
	$(CXX) $(CXXFLAGS) test_heap.cpp heap.cpp -o test_heap.exe


.PHONY: clean
clean:
	rm -f *.exe