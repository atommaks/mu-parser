#ifndef TYPES_H
#define TYPES_H

#include <set>
#include <unordered_set>
#include <map>
#include <queue>
#include <string>
#include <memory>
#include <vector>
#include <optional>

using namespace std;

struct Element {
    Element();
    Element(char c);
    operator char() const;
    bool is_terminal() const;
private:
    char c;
};

typedef Element Terminal;
typedef Element Variable;

typedef vector<Element> Word;

typedef set<Terminal> Alphabet;

struct Rule {

    Rule();
    Rule(Variable name);
    bool operator ==(const Rule &other) const;

    Variable name;
    Word production;
};

template <>
struct std::hash<Rule> {
    std::size_t operator()(const Rule& k) const;
};


struct CFG {
    Alphabet alphabet;
    vector<Rule> rules;
    Variable start;

    bool match(string word);
    vector<reference_wrapper<Rule> > rule(Variable name);
    set<Variable> variables();
    void print();

    static unique_ptr<CFG> from_mu_expression(string mu_expression);
    void simplify();
    void remove_epsilon_productions();
    void remove_unit_productions();

private:
    vector<Rule> make_epsilonless_rule(Rule rule, int pos, vector<Variable> epsilon_vars);
    void remove_unit_production(Rule rule);
    void remove_duplicates();
    void sort_rules();

    bool simplified = false;
    bool includes_empty_word = false;
};

struct CFGMatcher {
    CFGMatcher(CFG cfg, string word);
    bool matches();

private:
    CFG cfg;
    string word;
    int pos;

    optional<bool> result;

    struct MatchState {
        Rule rule;
        int pos;
        int origin;

        MatchState(Rule rule, int pos, int origin);
        bool operator ==(const MatchState &other) const;

        Element next() const;
        bool is_finished() const;
    };
    vector<unordered_set<MatchState>> states;

    void init();
    void match_iteration(MatchState state, int k);
    void add(MatchState state, int k);
    void predict(MatchState state, int k);
    void scan(MatchState state, int k);
    void complete(MatchState state, int k);

    friend hash<CFGMatcher::MatchState>;
};

template <>
struct std::hash<CFGMatcher::MatchState> {
    size_t operator()(const CFGMatcher::MatchState& k) const;
};

struct MuParser {
    MuParser(string expression);
    unique_ptr<CFG> getCFG();
private:
    string expression;
    int pos;
    char next_nonterminal;
    unique_ptr<CFG> cfg;
    map<Terminal, Variable> mu_parameters;

    void parse_expression();
    Variable parse_mu_expr();
    void parse_rule(Variable rule_name);

    void expect(string s);
    char next();
    bool is_mu_parameter(Terminal t);
};

#endif // TYPES_H
