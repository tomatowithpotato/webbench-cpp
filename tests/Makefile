SOURCE_FILES += $(wildcard ./*.cpp)

INCLUDE_DIR = -I ./

signal_multithread: test_signal_multithread.cpp
	g++ -ggdb test_signal_multithread.cpp -lpthread -o ./smTest.out

server: server.cpp
	g++ -ggdb server.cpp -lpthread -o ./server.out 