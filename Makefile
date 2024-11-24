# Default target
all: myfind

# Build the executable
myfind: main.cpp
	g++ main.cpp -o myfind

# Clean up build artifacts
clean:
	rm -f myfind

.PHONY: all clean
