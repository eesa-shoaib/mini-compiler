#include "codegen.hpp"

#include <fstream>
#include <iomanip>
#include <stdexcept>

using namespace std;

string CodeGen::newTemp() {
    return "t" + to_string(++tempCount);
}

string CodeGen::newLabel() {
    return "L" + to_string(++labelCount);
}

void CodeGen::emit(const string& line) {
    tac.push_back(line);
}

void CodeGen::addSymbol(const string& name, const string& kind, const string& type) {
    symbols.push_back(Symbol{name, kind, type, nextAddress, "-"});
    nextAddress += 4;
}

void CodeGen::writeOutputs(const string& tacPath, const string& symPath) const {
    ofstream tacOut(tacPath);
    if (!tacOut) throw runtime_error("cannot open " + tacPath);
    for (const string& line : tac) {
        tacOut << line << '\n';
    }

    ofstream symOut(symPath);
    if (!symOut) throw runtime_error("cannot open " + symPath);
    symOut << left
           << setw(16) << "Name"
           << setw(12) << "Kind"
           << setw(8)  << "Type"
           << setw(10) << "Address"
           << "Initial\n";
    symOut << string(58, '-') << '\n';
    for (const Symbol& symbol : symbols) {
        symOut << left
               << setw(16) << symbol.name
               << setw(12) << symbol.kind
               << setw(8)  << symbol.type
               << setw(10) << symbol.address
               << symbol.initial << '\n';
    }
}
