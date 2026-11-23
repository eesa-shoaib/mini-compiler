#ifndef CMM_CODEGEN_HPP
#define CMM_CODEGEN_HPP

#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

class CodeGen {
public:
    string newTemp();
    string newLabel();

    void emit(const string& line);
    void addSymbol(const string& name, const string& kind, const string& type,
                   bool isGlobal = false);

    const vector<string>& tacLines() const;
    map<string, string> symbolTypes() const;
    set<string> globalNames() const;

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
    set<string> globals;
    int nextAddress = 1000;
    int tempCount = 0;
    int labelCount = 0;
};

#endif
