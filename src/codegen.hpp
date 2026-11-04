#ifndef CMM_CODEGEN_HPP
#define CMM_CODEGEN_HPP

#include <string>
#include <vector>

using namespace std;

// Collects three-address code and symbol-table entries while the parser walks
// the source. It performs no syntax checking of its own.
class CodeGen {
public:
    string newTemp();
    string newLabel();

    void emit(const string& line);
    void addSymbol(const string& name, const string& kind, const string& type);

    void writeOutputs(const string& tacPath, const string& symPath) const;

private:
    struct Symbol {
        string name;
        string kind;   // function, parameter, variable
        string type;   // int, char, void
        int address;
        string initial;
    };

    vector<string> tac;
    vector<Symbol> symbols;
    int nextAddress = 1000;
    int tempCount = 0;
    int labelCount = 0;
};

#endif
