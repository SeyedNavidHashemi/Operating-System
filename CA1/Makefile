CXX = g++
CXXFLAGS = -Wall -std=c++11
DEPS = room.h

OBJS = server.o room.o


server: $(OBJS)
	$(CXX) $(CXXFLAGS) -o server $(OBJS)

server.o: server.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c server.cpp

room.o: room.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c room.cpp

clean:
	rm -f *.o server