#ifndef CMM_VM_HPP
#define CMM_VM_HPP

#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

class VMError : public runtime_error {
public:
    explicit VMError(const string& message);
};

// Executes the three-address code produced by CodeGen.
//
// Supported instructions:
//   func NAME / endfunc NAME      function boundaries
//   param NAME                    formal parameter declaration
//   LABEL:                        jump target
//   goto LABEL                    unconditional jump
//   ifFalse X goto LABEL          jump when X is zero
//   X = Y                         copy / constant load / char constant
//   X = Y op Z                    arithmetic and relational operations
//   in X                          read an integer from stdin
//   out X                         print X (characters for char variables)
//   push X                        evaluate an actual argument
//   call NAME, N                  call, discard result
//   X = call NAME, N              call, keep result in X
//   return [X]                    return from the current function
class VirtualMachine {
public:
    VirtualMachine(const vector<string>& tac,
                   const map<string, string>& types,
                   const set<string>& globalNames);

    void run();

private:
    struct Function {
        size_t begin = 0;
        size_t end = 0;
    };

    vector<string> code;
    map<string, size_t> labels;
    map<string, Function> functions;
    map<string, vector<string>> formals;
    map<string, string> types;
    map<string, int> globals;

    void load();
    int execute(const string& name, map<string, int>& locals);
    int call(const string& nameToken, const string& countToken,
             size_t pc, vector<int>& pending);
    void printOutput(const string& token, map<string, int>& locals) const;
    int operand(const string& token, map<string, int>& locals) const;
    void assign(const string& name, int value, map<string, int>& locals);
    int apply(const string& op, int left, int right, size_t pc) const;
    size_t label(const string& name) const;
    VMError fail(size_t pc, const string& message) const;

    static bool isCharLiteral(const string& token);
    static vector<string> split(const string& line);
};

#endif
