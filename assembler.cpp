#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace std;

// =========================================================================
// HELPERS & VALIDATION 
// =========================================================================

static string getFileBaseName(const string& path) {
    size_t dot = path.find_last_of('.');
    if (dot == string::npos) return path;
    return path.substr(0, dot);
}

static bool isValidLabel(const string& s) {
    if (s.empty() || !isalpha(static_cast<unsigned char>(s[0]))) return false;
    for (size_t i = 1; i < s.size(); i++) {
        char c = s[i];
        if (!isalnum(static_cast<unsigned char>(c)) && c != '_') return false;
    }
    return true;
}

static bool isDecimal(const string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!isdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

static bool isOctal(const string& s) {
    if (s.size() < 2 || s[0] != '0') return false;
    for (size_t i = 1; i < s.size(); i++) {
        if (s[i] < '0' || s[i] > '7') return false;
    }
    return true;
}

static bool isHex(const string& s) {
    if (s.size() < 3 || s[0] != '0' || tolower(static_cast<unsigned char>(s[1])) != 'x') return false;
    for (size_t i = 2; i < s.size(); i++) {
        if (!isxdigit(static_cast<unsigned char>(s[i]))) return false;
    }
    return true;
}

static bool isBranchInstruction(const string& inst) {
    return inst == "br" || inst == "brz" || inst == "brlz" || inst == "call";
}

static string decimalToHex(int value) {
    unsigned int v = static_cast<unsigned int>(value);
    const string digits = "0123456789ABCDEF";
    string result;

    do {
        result = digits[v % 16] + result;
        v /= 16;
    } while (v > 0);

    while (result.length() < 8) result = "0" + result;
    return result;
}

static int parseNumber(string s) {
    int sign = 1;
    if (!s.empty() && (s[0] == '-' || s[0] == '+')) {
        sign = (s[0] == '-') ? -1 : 1;
        s = s.substr(1);
    }

    if (isHex(s))   return sign * stoi(s, nullptr, 16);
    if (isOctal(s)) return sign * stoi(s, nullptr, 8);
    return sign * stoi(s);
}

// =========================================================================
// DATA STRUCTURES & GLOBAL STATE
// =========================================================================

struct Line {
    int pc = 0;
    int originalLine = 0;
    string label;
    string instruction;
    string operand;
};

struct Error {
    int line;
    string message;
};

struct Warning {
    int line;
    string message;
};

struct ListingLine {
    string address;
    string machineCode;
    string originalLine;
};

static vector<string> sourceLines;
static vector<Error> errors;
static vector<Warning> warnings;
static vector<Line> program;
static vector<ListingLine> listing;
static vector<string> machineCode;

static map<string, pair<string,int>> instructionTable;
static map<string, pair<int,int>> symbolTable;
static map<string, vector<int>> labelUsage;
static map<string, string> variables;

// =========================================================================
// PIPELINE STEPS
// =========================================================================

static void initInstructionTable() {
    instructionTable["data"]   = {"", 1};
    instructionTable["ldc"]    = {"00", 1};
    instructionTable["adc"]    = {"01", 1};
    instructionTable["ldl"]    = {"02", 2};
    instructionTable["stl"]    = {"03", 2};
    instructionTable["ldnl"]   = {"04", 2};
    instructionTable["stnl"]   = {"05", 2};
    instructionTable["add"]    = {"06", 0};
    instructionTable["sub"]    = {"07", 0};
    instructionTable["shl"]    = {"08", 0};
    instructionTable["shr"]    = {"09", 0};
    instructionTable["adj"]    = {"0A", 1};
    instructionTable["a2sp"]   = {"0B", 0};
    instructionTable["sp2a"]   = {"0C", 0};
    instructionTable["call"]   = {"0D", 2};
    instructionTable["return"] = {"0E", 0};
    instructionTable["brz"]    = {"0F", 2};
    instructionTable["brlz"]   = {"10", 2};
    instructionTable["br"]     = {"11", 2};
    instructionTable["HALT"]   = {"12", 0};
    instructionTable["SET"]    = {"", 1};
}

static bool readSourceFile(const string& path) {
    ifstream file(path);
    if (!file) return false;

    string line;
    while (getline(file, line)) sourceLines.push_back(line);
    return true;
}

static vector<string> tokenizeLine(string s) {
    // drop comments starting with ';'
    int comment = static_cast<int>(s.find(';'));
    if (comment != -1) s = s.substr(0, comment);

    // make sure "label:instruction" becomes "label: instruction"
    int colon = static_cast<int>(s.find(':'));
    if (colon != -1 && colon + 1 < static_cast<int>(s.size()) && s[colon + 1] != ' ') {
        s.insert(colon + 1, " ");
    }

    vector<string> tokens;
    string word;
    for (char c : s) {
        if (isspace(static_cast<unsigned char>(c))) {
            if (!word.empty()) {
                tokens.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) tokens.push_back(word);
    return tokens;
}

static void buildProgramFromSource() {
    for (size_t i = 0; i < sourceLines.size(); i++) {
        vector<string> tokens = tokenizeLine(sourceLines[i]);
        if (tokens.empty()) continue;

        Line line;
        line.originalLine = static_cast<int>(i) + 1;

        size_t idx = 0;
        if (!tokens.empty() && !tokens[idx].empty() && tokens[idx].back() == ':') {
            line.label = tokens[idx].substr(0, tokens[idx].size() - 1);
            idx++;
        }

        if (idx < tokens.size()) {
            line.instruction = tokens[idx];
            idx++;
        }

        if (idx < tokens.size()) {
            line.operand = tokens[idx];
            idx++;
        }

        if (idx < tokens.size()) {
            errors.push_back({static_cast<int>(i) + 1, "Extra operand"});
        }

        program.push_back(line);
    }
}

static void pass1BuildSymbols() {
    int pc = 0;
    bool hasHalt = false;

    for (size_t i = 0; i < program.size(); i++) {
        Line& line = program[i];
        line.pc = pc;
        int lineNum = line.originalLine;

        // label checks + symbol table
        if (!line.label.empty()) {
            if (!isValidLabel(line.label)) {
                errors.push_back({lineNum, "Invalid label name"});
            } else if (symbolTable.count(line.label) && symbolTable[line.label].first != -1) {
                errors.push_back({lineNum, "Duplicate label"});
            } else {
                if (line.instruction != "SET") {
                    symbolTable[line.label] = {pc, lineNum};
                }
            }
        }

        if (line.instruction.empty()) continue;
        if (line.instruction == "HALT") hasHalt = true;

        if (!instructionTable.count(line.instruction)) {
            errors.push_back({lineNum, "Unknown instruction"});
            pc++;
            continue;
        }

        int operandType = instructionTable[line.instruction].second;

        if (operandType > 0) {
            if (line.operand.empty()) {
                errors.push_back({lineNum, "Missing operand"});
            } else {
                string op = line.operand;
                if (!op.empty() && (op[0] == '+' || op[0] == '-')) op = op.substr(1);

                bool validNumber = isDecimal(op) || isOctal(op) || isHex(op);
                if (!isValidLabel(op) && !validNumber) {
                    errors.push_back({lineNum, "Invalid operand"});
                } else if (validNumber) {
                    try {
                        int val = parseNumber(op);
                        // Check 24-bit signed limits (-8388608 to 8388607)
                        if (val > 8388607 || val < -8388608) {
                            errors.push_back({lineNum, "Operand out of range (24-bit limit)"});
                        }
                    } catch (const std::out_of_range& e) {
                        errors.push_back({lineNum, "Operand massively out of range"});
                    }
                }
            }

            // SET is a pseudo-instruction that defines a value
            if (line.instruction == "SET") {
                if (line.label.empty()) {
                    errors.push_back({lineNum, "SET needs a label"});
                } else {
                    variables[line.label] = line.operand;
                }
            } else {
                pc++;
            }
        } else {
            if (!line.operand.empty()) {
                errors.push_back({lineNum, "Unexpected operand"});
            }
            pc++;
        }
    }

    // after pass 1, check for undefined or unused labels
    for (auto it = symbolTable.begin(); it != symbolTable.end(); it++) {
        const string& label = it->first;
        int address = it->second.first;
        int declaredLine = it->second.second;

        if (address == -1) {
            if (variables.count(label) == 0) {
                for (int lineUsed : labelUsage[label]) {
                    errors.push_back({lineUsed, "No such label"});
                }
            }
        } else if (!labelUsage.count(label) && !variables.count(label)) {
            warnings.push_back({declaredLine, "Unused label"});
        }
    }

    if (!hasHalt) warnings.push_back({0, "No HALT instruction"});
}

static void pass2GenerateOutput() {
    if (!errors.empty()) return;

    for (size_t i = 0; i < program.size(); i++) {
        Line& line = program[i];

        ListingLine row;
        row.address = decimalToHex(line.pc);
        row.originalLine = sourceLines[line.originalLine - 1];

        if (line.instruction.empty()) {
            row.machineCode = "        ";
            listing.push_back(row);
            continue;
        }
 
        string opcode = instructionTable[line.instruction].first;
        int type = instructionTable[line.instruction].second;
        int value = 0;

        if (type > 0) {
            if (isValidLabel(line.operand)) {
                if (variables.count(line.operand)) {
                    value = parseNumber(variables[line.operand]);
                } else {
                    value = symbolTable[line.operand].first;
                }

                if (isBranchInstruction(line.instruction)) {
                    value = value - (line.pc + 1);
                }
            } else {
                value = parseNumber(line.operand);
            }
        }

        if (line.instruction == "data" || line.instruction == "SET") {
            row.machineCode = decimalToHex(value);
            if (line.instruction == "data") machineCode.push_back(row.machineCode);
        } else if (type == 0) {
            row.machineCode = "000000" + opcode;
            machineCode.push_back(row.machineCode);
        } else {
            string full = decimalToHex(value);
            string operand24 = full.substr(2, 6);
            row.machineCode = operand24 + opcode;
            machineCode.push_back(row.machineCode);
        }

        listing.push_back(row);
    }
}

static void writeOutputs(const string& fileBaseName, const string& sourcePath) {
    sort(errors.begin(), errors.end(),
         [](const Error& a, const Error& b) { return a.line < b.line; });

    sort(warnings.begin(), warnings.end(),
         [](const Warning& a, const Warning& b) { return a.line < b.line; });

    ofstream logFile(fileBaseName + ".log");
    for (auto& w : warnings) {
        logFile << "Line " << w.line << " WARNING: " << w.message << "\n";
    }
    for (auto& e : errors) {
        logFile << "Line " << e.line << " ERROR: " << e.message << "\n";
    }
    if (errors.empty()) {
        logFile << "Assembly completed successfully with no errors.\n";
    }

    ofstream lstFile(fileBaseName + ".lst");
    for (auto& row : listing) {
        lstFile << row.address << " " << row.machineCode << " " << row.originalLine << "\n";
    }

    if (errors.empty()) {
        ofstream objFile(fileBaseName + ".o", ios::binary);
        for (auto& code : machineCode) {
            unsigned int value = stoul(code, nullptr, 16);
            objFile.write(reinterpret_cast<char*>(&value), sizeof(value));
        }

        cout << "Successfully assembled " << sourcePath << " into " << fileBaseName << ".o\n";
    } else {
        cout << "Assembly failed with " << errors.size() << " error(s). Check "
             << fileBaseName << ".log\n";
    }
}

// =========================================================================
// MAIN
// =========================================================================

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: ./asm <file.asm>\n";
        return 0;
    }

    const string sourcePath = argv[1];
    const string fileBaseName = getFileBaseName(sourcePath);

    initInstructionTable();

    if (!readSourceFile(sourcePath)) {
        cout << "Cannot open file\n";
        return 0;
    }

    buildProgramFromSource();
    pass1BuildSymbols();
    pass2GenerateOutput();
    writeOutputs(fileBaseName, sourcePath);

    return 0;
}
