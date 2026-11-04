#include "parser.hpp"

#include <fstream>
#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <source-file.cmm>\n";
        return 1;
    }

    ifstream file(argv[1]);
    if (!file) {
        cerr << "Error: cannot open file " << argv[1] << "\n";
        return 1;
    }

    try {
        Parser parser(file);
        parser.parse();
        parser.codegen().writeOutputs("tac.txt", "symtab.txt");
        cout << "Parsing successful.\n"
             << "TAC written to tac.txt\n"
             << "Symbol table written to symtab.txt\n";
    } catch (const ParseError& error) {
        cerr << error.what() << '\n';
        return 1;
    } catch (const exception& error) {
        cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
