CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic

all: asm emu

asm: assembler.cpp
	$(CXX) $(CXXFLAGS) assembler.cpp -o asm

emu: emulator.cpp
	$(CXX) $(CXXFLAGS) emulator.cpp -o emu

clean:
	rm -f asm emu *.o *.log *.lst