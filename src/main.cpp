#include "parser.hpp"
#include "vm.hpp"

#include <fstream>
#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
    bool run = false;
    string source;

    for (int i = 1; i < argc; ++i) {
        const string argument = argv[i];
        if (argument == "--run") {
            run = true;
        } else if (!argument.empty() && argument[0] == '-') {
            cerr << "Unknown option: " << argument << "\n";
            return 1;
        } else if (source.empty()) {
            source = argument;
        } else {
            cerr << "Only one source file may be given\n";
            return 1;
        }
    }

    if (source.empty()) {
        cerr << "Usage: " << argv[0] << " [--run] <source-file.cmm>\n";
        return 1;
    }

    ifstream file(source);
    if (!file) {
        cerr << "Error: cannot open file " << source << "\n";
        return 1;
    }

    try {
        Parser parser(file);
        parser.parse();

        CodeGen& gen = parser.codegen();
        gen.writeOutputs("tac.txt", "symtab.txt");
        cout << "Parsing successful.\n"
             << "TAC written to tac.txt\n"
             << "Symbol table written to symtab.txt\n";

        if (run) {
            VirtualMachine vm(gen.tacLines(), gen.symbolTypes(), gen.globalNames());
            vm.run();
        }
    } catch (const ParseError& error) {
        cerr << error.what() << '\n';
        return 1;
    } catch (const VMError& error) {
        cerr << "Runtime error: " << error.what() << '\n';
        return 1;
    } catch (const exception& error) {
        cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
