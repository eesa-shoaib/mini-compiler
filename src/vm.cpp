#include "vm.hpp"

#include <cctype>
#include <iostream>

using namespace std;

VMError::VMError(const string& message) : runtime_error(message) {}

VirtualMachine::VirtualMachine(const vector<string>& tac,
                               const map<string, string>& types,
                               const set<string>& globalNames)
    : code(tac), types(types) {
    for (const string& name : globalNames) {
        globals[name] = 0;
    }
    load();
}

vector<string> VirtualMachine::split(const string& line) {
    vector<string> tokens;
    string token;
    bool quoted = false;
    for (char c : line) {
        if (c == '\'') {
            quoted = !quoted;
            token += c;
        } else if (isspace((unsigned char)c) && !quoted) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }
    if (!token.empty()) tokens.push_back(token);
    return tokens;
}

void VirtualMachine::load() {
    string openFunction;
    for (size_t i = 0; i < code.size(); ++i) {
        const vector<string> tokens = split(code[i]);
        if (tokens.empty()) continue;
        if (tokens.size() == 2 && tokens[0] == "func") {
            if (!openFunction.empty()) {
                throw VMError("nested function definition '" + tokens[1] + "'");
            }
            openFunction = tokens[1];
            functions[openFunction].begin = i + 1;
        } else if (tokens.size() == 2 && tokens[0] == "endfunc") {
            if (openFunction != tokens[1]) {
                throw VMError("endfunc '" + tokens[1] + "' does not match func '" +
                              openFunction + "'");
            }
            functions[tokens[1]].end = i;
            openFunction.clear();
        } else if (tokens.size() == 2 && tokens[0] == "param") {
            if (openFunction.empty()) throw VMError("param outside a function");
            formals[openFunction].push_back(tokens[1]);
        } else if (tokens.size() == 1 && tokens[0].back() == ':') {
            labels[tokens[0].substr(0, tokens[0].size() - 1)] = i;
        }
    }
    if (!openFunction.empty()) {
        throw VMError("missing endfunc for '" + openFunction + "'");
    }
}

void VirtualMachine::run() {
    if (functions.find("main") == functions.end()) {
        throw VMError("no 'main' function in the TAC");
    }
    map<string, int> locals;
    execute("main", locals);
}

int VirtualMachine::execute(const string& name, map<string, int>& locals) {
    const Function& function = functions.at(name);
    size_t pc = function.begin;
    vector<int> pending;

    while (pc < function.end) {
        const size_t line = pc++;
        const vector<string> tokens = split(code[line]);
        if (tokens.empty() || tokens[0].back() == ':') continue;
        const string& op = tokens[0];

        if (op == "goto") {
            pc = label(tokens[1]);
        } else if (op == "ifFalse") {
            if (operand(tokens[1], locals) == 0) pc = label(tokens[3]);
        } else if (op == "in") {
            int input;
            if (!(cin >> input)) throw fail(line, "expected an integer input");
            assign(tokens[1], input, locals);
        } else if (op == "out") {
            printOutput(tokens[1], locals);
        } else if (op == "push") {
            pending.push_back(operand(tokens[1], locals));
        } else if (op == "call") {
            call(tokens[1], tokens[2], line, pending);
        } else if (op == "return") {
            return tokens.size() > 1 ? operand(tokens[1], locals) : 0;
        } else if (op == "param") {
            continue;
        } else if (tokens.size() >= 3 && tokens[1] == "=") {
            if (tokens.size() == 5 && tokens[2] == "call") {
                assign(tokens[0], call(tokens[3], tokens[4], line, pending), locals);
            } else if (tokens.size() == 3) {
                assign(tokens[0], operand(tokens[2], locals), locals);
            } else if (tokens.size() == 5) {
                assign(tokens[0],
                       apply(tokens[3],
                             operand(tokens[2], locals),
                             operand(tokens[4], locals),
                             line),
                       locals);
            } else {
                throw fail(line, "unknown TAC instruction");
            }
        } else {
            throw fail(line, "unknown TAC instruction");
        }
    }
    return 0;
}

int VirtualMachine::call(const string& nameToken, const string& countToken,
                         size_t pc, vector<int>& pending) {
    string name = nameToken;
    if (!name.empty() && name.back() == ',') name.pop_back();

    int count;
    try {
        count = stoi(countToken);
    } catch (const exception&) {
        throw fail(pc, "bad argument count '" + countToken + "'");
    }
    if (count < 0 || static_cast<size_t>(count) > pending.size()) {
        throw VMError("not enough arguments pushed for call");
    }

    map<string, Function>::const_iterator entry = functions.find(name);
    if (entry == functions.end()) {
        throw VMError("call to undefined function '" + name + "'");
    }
    const vector<string>& parameters = formals[name];
    if (parameters.size() != static_cast<size_t>(count)) {
        throw VMError("function '" + name + "' expects " + to_string(parameters.size()) +
                      " argument(s) but got " + to_string(count));
    }

    map<string, int> locals;
    for (int i = 0; i < count; ++i) {
        locals[parameters[i]] = pending[pending.size() - count + i];
    }
    pending.resize(pending.size() - count);
    return execute(name, locals);
}

void VirtualMachine::printOutput(const string& token, map<string, int>& locals) const {
    if (isCharLiteral(token)) {
        cout << token[1] << '\n';
        return;
    }
    const int result = operand(token, locals);
    if (types.count(token) && types.at(token) == "char") {
        cout << static_cast<char>(result) << '\n';
    } else {
        cout << result << '\n';
    }
}

int VirtualMachine::operand(const string& token, map<string, int>& locals) const {
    if (isCharLiteral(token)) {
        return static_cast<unsigned char>(token[1]);
    }

    size_t start = (!token.empty() && token[0] == '-') ? 1 : 0;
    bool numeric = start < token.size();
    for (size_t i = start; numeric && i < token.size(); ++i) {
        if (!isdigit(static_cast<unsigned char>(token[i]))) numeric = false;
    }
    if (numeric) return stoi(token);

    map<string, int>::const_iterator local = locals.find(token);
    if (local != locals.end()) return local->second;

    map<string, int>::const_iterator global = globals.find(token);
    if (global != globals.end()) return global->second;

    return 0;
}

void VirtualMachine::assign(const string& name, int value, map<string, int>& locals) {
    if (locals.find(name) != locals.end()) {
        locals[name] = value;
    } else if (globals.find(name) != globals.end()) {
        globals[name] = value;
    } else {
        locals[name] = value;
    }
}

int VirtualMachine::apply(const string& op, int left, int right, size_t pc) const {
    if (op == "+") return left + right;
    if (op == "-") return left - right;
    if (op == "*") return left * right;
    if (op == "/") {
        if (right == 0) throw fail(pc, "division by zero");
        return left / right;
    }
    if (op == "%") {
        if (right == 0) throw fail(pc, "modulo by zero");
        return left % right;
    }
    if (op == "<") return left < right;
    if (op == ">") return left > right;
    if (op == "<=") return left <= right;
    if (op == ">=") return left >= right;
    if (op == "==") return left == right;
    if (op == "!=") return left != right;
    throw fail(pc, "unknown operator '" + op + "'");
}

size_t VirtualMachine::label(const string& name) const {
    map<string, size_t>::const_iterator entry = labels.find(name);
    if (entry == labels.end()) {
        throw VMError("unknown label '" + name + "'");
    }
    return entry->second;
}

VMError VirtualMachine::fail(size_t pc, const string& message) const {
    return VMError(message + " near: " + code[pc]);
}

bool VirtualMachine::isCharLiteral(const string& token) {
    return token.size() >= 2 && token.front() == '\'' && token.back() == '\'';
}
