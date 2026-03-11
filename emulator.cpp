#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <sstream>
#include <algorithm>

using namespace std;

// =========================================================================
// VIRTUAL HARDWARE & STATE
// =========================================================================

// Registers
int A = 0;  // Accumulator
int B = 0;  // Internal Stack Register
int PC = 0; // Program Counter
int SP = 0; // Stack Pointer

// Memory: 24-bit operands => 2^24 word address space
vector<int> memory(1 << 24, 0);

// Execution state
int instructionCount = 0;
bool isHalted = false;
int maxMemoryUsed = 0; // Highest memory address written to

// =========================================================================
// UTILITIES
// =========================================================================

static string toHex(int value) {
    stringstream ss;
    ss << setfill('0') << setw(8) << hex << uppercase << (unsigned int)value;
    return ss.str();
}

static int signExtend24(int value) {
    // manual 24-bit sign extension so negatives behave the same everywhere
    if (value & 0x00800000) {
        value |= 0xFF000000;
    }
    return value;
}

static void resetRegisters() {
    A = 0;
    B = 0;
    PC = 0;
    SP = 0;
}

// =========================================================================
// CPU CORE (Fetch, Decode, Execute)
// =========================================================================

static void executeSingleCycle() {
    // Fetch
    int instruction = memory[PC];
    int oldPC = PC;
    PC++;

    // Decode
    int opcode = instruction & 0xFF;
    int value = instruction >> 8;
    value = signExtend24(value);

    // Execute
    switch (opcode) {
        case 0:  // ldc
            B = A; A = value; break;
        case 1:  // adc
            A = A + value; break;
        case 2:  // ldl
            B = A; A = memory[SP + value]; break;
        case 3:  // stl
            memory[SP + value] = A; A = B;
            maxMemoryUsed = max(maxMemoryUsed, (SP + value) + 1);
            break;
        case 4:  // ldnl
            A = memory[A + value]; break;
        case 5:  // stnl
            memory[A + value] = B;
            maxMemoryUsed = max(maxMemoryUsed, (A + value) + 1);
            break;
        case 6:  // add
            A = B + A; break;
        case 7:  // sub
            A = B - A; break;
        case 8:  // shl
            A = B << A; break;
        case 9:  // shr
            A = B >> A; break;
        case 10: // adj
            SP = SP + value; break;
        case 11: // a2sp
            SP = A; A = B; break;
        case 12: // sp2a
            B = A; A = SP; break;
        case 13: // call
            B = A; A = PC; PC = PC + value; break;
        case 14: // return
            PC = A; A = B; break;
        case 15: // brz
            if (A == 0) PC = PC + value;
            break;
        case 16: // brlz
            if (A < 0) PC = PC + value;
            break;
        case 17: // br
            PC = PC + value; break;
        case 18: // HALT
            isHalted = true; break;
        default:
            cout << "Warning: Unknown opcode " << opcode << " at PC " << toHex(oldPC) << "\n";
            isHalted = true;
            break;
    }

    instructionCount++;

    // Safety check: if we run off into nowhere, stop.
    if (PC < 0 || PC > maxMemoryUsed) {
        cout << "Error: CPU wandered into uninitialized memory (Did you forget a HALT instruction?).\n";
        isHalted = true;
    }

    // quick and dirty infinite loop catch
    if (opcode == 17 && value == -1) {
        cout << "Error: Infinite Loop Detected at PC " << toHex(oldPC) << ".\n";
        isHalted = true;
    }
}

// =========================================================================
// EXECUTION LOOP & FILE I/O
// =========================================================================

static void loadMachineCode(const string& filename) {
    ifstream file(filename, ios::in | ios::binary);
    if (!file) {
        cout << "Error: Cannot open object file " << filename << "\n";
        exit(0);
    }

    unsigned int currentInstruction;
    int memoryAddress = 0;

    while (file.read(reinterpret_cast<char*>(&currentInstruction), sizeof(int))) {
        memory[memoryAddress] = currentInstruction;
        memoryAddress++;
    }

    maxMemoryUsed = max(maxMemoryUsed, memoryAddress);
}

static void dumpMemory() {
    cout << "\nMemory Dump:\n";
    for (int i = 0; i < maxMemoryUsed; i += 4) {
        cout << toHex(i) << " ";
        for (int j = i; j < min(maxMemoryUsed, i + 4); ++j) {
            cout << toHex(memory[j]) << " ";
        }
        cout << "\n";
    }
    cout << "\n";
}

static void displayISA() {
    cout << "Opcode  Mnemonic  Operand\n";
    cout << "00      ldc       value\n";
    cout << "01      adc       value\n";
    cout << "02      ldl       value\n";
    cout << "03      stl       value\n";
    cout << "04      ldnl      value\n";
    cout << "05      stnl      value\n";
    cout << "06      add       \n";
    cout << "07      sub       \n";
    cout << "08      shl       \n";
    cout << "09      shr       \n";
    cout << "0A      adj       value\n";
    cout << "0B      a2sp      \n";
    cout << "0C      sp2a      \n";
    cout << "0D      call      offset\n";
    cout << "0E      return    \n";
    cout << "0F      brz       offset\n";
    cout << "10      brlz      offset\n";
    cout << "11      br        offset\n";
    cout << "12      HALT      \n";
    cout << "        SET       value\n";
}

static void printUsage() {
    cout << "Usage: ./emu [command] <filename.o>\n";
    cout << "Commands:\n";
    cout << "  -trace   Show register states per instruction\n";
    cout << "  -before  Show memory dump before execution\n";
    cout << "  -after   Show memory dump after execution\n";
    cout << "  -wipe    Reset all registers to zero before execution\n";
    cout << "  -isa     Display the Instruction Set Architecture\n";
}

// =========================================================================
// MAIN
// =========================================================================

int main(int argc, char* argv[]) {
    if (argc < 3 && !(argc == 2 && string(argv[1]) == "-isa")) {
        printUsage();
        return 0;
    }

    string command = argv[1];

    if (command == "-isa") {
        displayISA();
        return 0;
    }

    string filename = argv[2];
    loadMachineCode(filename);

    if (command == "-wipe") {
        resetRegisters();
    }

    if (command == "-before") {
        dumpMemory();
    }

    bool traceMode = (command == "-trace");

    while (!isHalted) {
        if (traceMode) {
            cout << "PC=" << toHex(PC) << " SP=" << toHex(SP)
                 << " A=" << toHex(A) << " B=" << toHex(B) << "\n";
        }
        executeSingleCycle();
    }

    if (command == "-after") {
        dumpMemory();
    }

    cout << instructionCount << " instructions executed.\n";
    return 0;
}
