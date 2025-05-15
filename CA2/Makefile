CXX = g++

CXXFLAGS = -Wall -Wextra -std=c++17

TARGETS = main product warehouse

all: $(TARGETS)

main: main.cpp
	$(CXX) $(CXXFLAGS) -o main main.cpp

product: product.cpp
	$(CXX) $(CXXFLAGS) -o product product.cpp

warehouse: warehouse.cpp
	$(CXX) $(CXXFLAGS) -o warehouse warehouse.cpp

clean:
	rm -f $(TARGETS)

.PHONY: all clean
