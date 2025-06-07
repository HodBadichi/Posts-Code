CXX = g++
CXXFLAGS = -Wall -Wextra -O3 -g
TARGET = binary_search_benchmark

all: $(TARGET)

$(TARGET): binary_search.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) binary_search.cpp

clean:
	rm -f $(TARGET)

.PHONY: all clean 