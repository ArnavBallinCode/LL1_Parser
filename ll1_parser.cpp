/*
    =========================================================
                 LL(1) PARSER  -
    =========================================================

    What does this program do?  (Explained like you are 5)

    1. We write down a small GRAMMAR (the "rules" of a tiny language
       of math expressions like  id + id * id ).
    2. We find FIRST of every rule-name  -> "which word can come first?"
    3. We find FOLLOW of every rule-name -> "which word can come right after?"
    4. Using FIRST and FOLLOW we fill a PARSING TABLE
       -> "if I am at rule X and I see word a, which rule do I use?"
    5. We PARSE an input using a STACK and the table, step by step,
       and say ACCEPTED or REJECTED.

    Grammar used (classic expression grammar, no left recursion):

        E  -> T E'
        E' -> + T E' | eps
        T  -> F T'
        T' -> * F T' | eps
        F  -> ( E ) | id

    "eps" means epsilon = empty = "nothing".
    "$"   means end of input.

    Compile :  g++ ll1_parser.cpp -o ll1_parser
    Run     :  ./ll1_parser
*/

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <set>
using namespace std;

// ---------------------------------------------------------
// Some fixed words we use everywhere
// ---------------------------------------------------------
const string EPS = "eps";   // epsilon (empty)
const string END = "$";     // end of input marker

// ---------------------------------------------------------
// One grammar rule looks like:   left -> right[0] right[1] ...
// Example:  E -> T E'   means left = "E", right = {"T", "E'"}
// ---------------------------------------------------------
struct Rule {
    string left;
    vector<string> right;
};

// ---------------------------------------------------------
// Global data (kept global so every function can see it easily)
// ---------------------------------------------------------
vector<Rule>   rules;          // all grammar rules
vector<string> nonTerminals;   // E, E', T, T', F
vector<string> terminals;      // +, *, (, ), id
string         startSymbol;    // E

map<string, set<string>> FIRST;    // FIRST["E"]  = { (, id }
map<string, set<string>> FOLLOW;   // FOLLOW["E"] = { ), $ }

// table[A][a] = number of the rule to use
// when the top of stack is A and the input word is a
map<string, map<string, int>> table;

// =========================================================
//  PART 1 : THE GRAMMAR
// =========================================================

// Add one rule to the list
void addRule(string left, vector<string> right) {
    Rule r;
    r.left = left;
    r.right = right;
    rules.push_back(r);
}

// Write down our grammar
void setupGrammar() {
    startSymbol = "E";

    nonTerminals = {"E", "E'", "T", "T'", "F"};
    terminals    = {"+", "*", "(", ")", "id"};

    addRule("E",  {"T", "E'"});         // rule 0
    addRule("E'", {"+", "T", "E'"});    // rule 1
    addRule("E'", {EPS});               // rule 2
    addRule("T",  {"F", "T'"});         // rule 3
    addRule("T'", {"*", "F", "T'"});    // rule 4
    addRule("T'", {EPS});               // rule 5
    addRule("F",  {"(", "E", ")"});     // rule 6
    addRule("F",  {"id"});              // rule 7
}

// Is this symbol a non-terminal (a rule name like E, T, F) ?
bool isNonTerminal(string symbol) {
    for (int i = 0; i < nonTerminals.size(); i++) {
        if (nonTerminals[i] == symbol) {
            return true;
        }
    }
    return false;
}

// Turn rule number i into a nice string like "E -> T E'"
string ruleToString(int i) {
    string text = rules[i].left + " -> ";
    for (int j = 0; j < rules[i].right.size(); j++) {
        text = text + rules[i].right[j];
        if (j != rules[i].right.size() - 1) {
            text = text + " ";
        }
    }
    return text;
}

// Turn a set like {a, b} into the string "{ a, b }"
string setToString(set<string> s) {
    string text = "{ ";
    int count = 0;
    for (string item : s) {
        text = text + item;
        count++;
        if (count != s.size()) {
            text = text + ", ";
        }
    }
    text = text + " }";
    return text;
}

void printGrammar() {
    cout << "\n================ GRAMMAR ================\n";
    for (int i = 0; i < rules.size(); i++) {
        cout << "  (" << i << ")  " << ruleToString(i) << "\n";
    }
}

// =========================================================
//  PART 2 : FIRST
// =========================================================
//
//  FIRST of a list of symbols  X1 X2 X3 ...
//    - If X1 is a terminal  -> answer is just { X1 }
//    - If X1 is a non-terminal -> take FIRST(X1) (without eps)
//        and if X1 can become empty, also look at X2, then X3 ...
//    - If ALL of them can become empty -> add eps
//
//  "start" tells us from which position in the list to begin.
//  (We need this for FOLLOW, where we look at the part AFTER a symbol.)
// ---------------------------------------------------------
set<string> firstOfList(vector<string> symbols, int start) {
    set<string> result;
    bool allCanBeEmpty = true;

    for (int i = start; i < symbols.size(); i++) {
        string X = symbols[i];

        // eps by itself adds nothing, just move on
        if (X == EPS) {
            continue;
        }

        // A terminal: it is the first word. Stop here.
        if (!isNonTerminal(X)) {
            result.insert(X);
            allCanBeEmpty = false;
            break;
        }

        // A non-terminal: copy its FIRST set (but not eps)
        for (string s : FIRST[X]) {
            if (s != EPS) {
                result.insert(s);
            }
        }

        // If X can NOT become empty, we stop here.
        if (FIRST[X].count(EPS) == 0) {
            allCanBeEmpty = false;
            break;
        }
        // otherwise X can vanish, so keep looking at the next symbol
    }

    if (allCanBeEmpty) {
        result.insert(EPS);
    }
    return result;
}

// Keep going over all rules again and again
// until no FIRST set grows any more.
void computeFirst() {
    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < rules.size(); i++) {
            string A = rules[i].left;
            set<string> f = firstOfList(rules[i].right, 0);

            for (string s : f) {
                if (FIRST[A].count(s) == 0) {   // new item found
                    FIRST[A].insert(s);
                    changed = true;
                }
            }
        }
    }
}

void printFirst() {
    cout << "\n================ FIRST SETS ================\n";
    for (int i = 0; i < nonTerminals.size(); i++) {
        string A = nonTerminals[i];
        cout << "  FIRST(" << A << ")" << setw(5 - A.size()) << ""
             << "= " << setToString(FIRST[A]) << "\n";
    }
}

// =========================================================
//  PART 3 : FOLLOW
// =========================================================
//
//  Rules for FOLLOW:
//   1. Put $ in FOLLOW(start symbol).
//   2. For a rule  A -> ... B rest
//        add FIRST(rest) (without eps) to FOLLOW(B)
//   3. If rest can become empty (or there is no rest),
//        add FOLLOW(A) to FOLLOW(B)
//  Repeat until nothing changes.
// ---------------------------------------------------------
void computeFollow() {
    FOLLOW[startSymbol].insert(END);

    bool changed = true;
    while (changed) {
        changed = false;

        for (int i = 0; i < rules.size(); i++) {
            string A = rules[i].left;
            vector<string> right = rules[i].right;

            for (int j = 0; j < right.size(); j++) {
                string B = right[j];
                if (!isNonTerminal(B)) {
                    continue;   // FOLLOW is only for non-terminals
                }

                // FIRST of everything after B
                set<string> f = firstOfList(right, j + 1);

                // Step 2: add FIRST(rest) without eps
                for (string s : f) {
                    if (s != EPS && FOLLOW[B].count(s) == 0) {
                        FOLLOW[B].insert(s);
                        changed = true;
                    }
                }

                // Step 3: rest can vanish -> add FOLLOW(A)
                if (f.count(EPS) > 0) {
                    for (string s : FOLLOW[A]) {
                        if (FOLLOW[B].count(s) == 0) {
                            FOLLOW[B].insert(s);
                            changed = true;
                        }
                    }
                }
            }
        }
    }
}

void printFollow() {
    cout << "\n================ FOLLOW SETS ================\n";
    for (int i = 0; i < nonTerminals.size(); i++) {
        string A = nonTerminals[i];
        cout << "  FOLLOW(" << A << ")" << setw(5 - A.size()) << ""
             << "= " << setToString(FOLLOW[A]) << "\n";
    }
}

// =========================================================
//  PART 4 : PARSING TABLE
// =========================================================
//
//  For every rule  A -> alpha :
//    1. For each terminal a in FIRST(alpha) :  table[A][a] = this rule
//    2. If eps is in FIRST(alpha) :
//         for each b in FOLLOW(A) :            table[A][b] = this rule
//
//  If one cell gets TWO different rules -> the grammar is NOT LL(1).
// ---------------------------------------------------------
bool isLL1 = true;

void putInTable(string A, string a, int ruleNumber) {
    if (table[A].count(a) > 0 && table[A][a] != ruleNumber) {
        cout << "  CONFLICT at [" << A << ", " << a << "] : "
             << ruleToString(table[A][a]) << "  vs  "
             << ruleToString(ruleNumber) << "\n";
        isLL1 = false;
    }
    table[A][a] = ruleNumber;
}

void buildTable() {
    for (int i = 0; i < rules.size(); i++) {
        string A = rules[i].left;
        set<string> f = firstOfList(rules[i].right, 0);

        // Step 1
        for (string a : f) {
            if (a != EPS) {
                putInTable(A, a, i);
            }
        }

        // Step 2
        if (f.count(EPS) > 0) {
            for (string b : FOLLOW[A]) {
                putInTable(A, b, i);
            }
        }
    }
}

void printTable() {
    cout << "\n================ LL(1) PARSING TABLE ================\n";

    // columns = all terminals + $
    vector<string> columns = terminals;
    columns.push_back(END);

    // header row
    cout << left << setw(6) << "";
    for (int j = 0; j < columns.size(); j++) {
        cout << setw(15) << columns[j];
    }
    cout << "\n";

    // one row per non-terminal
    for (int i = 0; i < nonTerminals.size(); i++) {
        string A = nonTerminals[i];
        cout << setw(6) << A;
        for (int j = 0; j < columns.size(); j++) {
            string a = columns[j];
            if (table[A].count(a) > 0) {
                cout << setw(15) << ruleToString(table[A][a]);
            } else {
                cout << setw(15) << "-";     // empty cell = error
            }
        }
        cout << "\n";
    }

    if (isLL1) {
        cout << "\n  No conflicts -> the grammar IS LL(1).\n";
    } else {
        cout << "\n  Conflicts found -> the grammar is NOT LL(1).\n";
    }
}

// =========================================================
//  PART 5 : TOKENIZER (break input text into words)
// =========================================================
//
//  "a + b*c"  becomes   id  +  id  *  id  $
//  Any name or number (letters / digits) is treated as "id".
// ---------------------------------------------------------
vector<string> tokenize(string line) {
    vector<string> tokens;
    int i = 0;

    while (i < line.size()) {
        char c = line[i];

        if (c == ' ') {                 // skip spaces
            i++;
        } else if (isalnum(c)) {        // a name or number
            while (i < line.size() && isalnum(line[i])) {
                i++;
            }
            tokens.push_back("id");
        } else {                        // a single symbol like + * ( )
            tokens.push_back(string(1, c));
            i++;
        }
    }

    tokens.push_back(END);   // always end with $
    return tokens;
}

// =========================================================
//  PART 6 : THE PARSER (stack + table)
// =========================================================
//
//  Start: stack = [ $, E ]   input = tokens ... $
//  Loop:
//    top = top of stack, current = current input word
//    - top == $ and current == $       -> ACCEPT
//    - top is a terminal:
//         same as current  -> pop it and move to next word (MATCH)
//         different        -> ERROR
//    - top is a non-terminal:
//         look in table[top][current]
//         empty  -> ERROR
//         rule   -> pop top, push the rule's right side in REVERSE
//                   (reverse so that the first symbol ends up on top)
//
//  We use a vector as the stack (back() = top) so we can print it.
// ---------------------------------------------------------

string stackToString(vector<string> st) {
    string text = "";
    for (int i = 0; i < st.size(); i++) {
        text = text + st[i] + " ";
    }
    return text;
}

string inputToString(vector<string> tokens, int pos) {
    string text = "";
    for (int i = pos; i < tokens.size(); i++) {
        text = text + tokens[i] + " ";
    }
    return text;
}

bool parse(string line) {
    vector<string> tokens = tokenize(line);

    vector<string> st;          // the stack
    st.push_back(END);
    st.push_back(startSymbol);

    int pos = 0;                // index of current input word

    cout << "\n--------------------------------------------------------------\n";
    cout << " Parsing: " << line << "\n";
    cout << "--------------------------------------------------------------\n";
    cout << left << setw(22) << " STACK" << setw(22) << "INPUT" << "ACTION\n";

    while (true) {
        string top = st.back();
        string current = tokens[pos];

        cout << " " << setw(21) << stackToString(st)
             << setw(22) << inputToString(tokens, pos);

        // Case 1: both finished -> success
        if (top == END && current == END) {
            cout << "ACCEPT\n";
            cout << " Result: STRING ACCEPTED\n";
            return true;
        }

        // Case 2: top is a terminal (or $)
        if (!isNonTerminal(top)) {
            if (top == current) {
                cout << "match " << current << "\n";
                st.pop_back();
                pos++;
            } else {
                cout << "ERROR: expected '" << top << "' but got '"
                     << current << "'\n";
                cout << " Result: STRING REJECTED\n";
                return false;
            }
            continue;
        }

        // Case 3: top is a non-terminal -> ask the table
        if (table[top].count(current) == 0) {
            cout << "ERROR: no rule for [" << top << ", " << current << "]\n";
            cout << " Result: STRING REJECTED\n";
            return false;
        }

        int r = table[top][current];
        cout << ruleToString(r) << "\n";

        st.pop_back();
        vector<string> right = rules[r].right;
        for (int k = right.size() - 1; k >= 0; k--) {
            if (right[k] != EPS) {      // never push eps
                st.push_back(right[k]);
            }
        }
    }
}

// =========================================================
//  MAIN
// =========================================================
int main() {
    setupGrammar();
    printGrammar();

    computeFirst();
    printFirst();

    computeFollow();
    printFollow();

    buildTable();
    printTable();

    // A few ready-made examples
    parse("id + id * id");
    parse("( id + id ) * id");
    parse("id + * id");

    // Let the user try their own expressions
    cout << "\nType an expression (example: a + b * c), or 'exit' to quit.\n";
    string line;
    while (true) {
        cout << "\n> ";
        if (!getline(cin, line) || line == "exit") {
            break;
        }
        if (line == "") {
            continue;
        }
        parse(line);
    }

    return 0;
}
