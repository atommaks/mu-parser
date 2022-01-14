
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <cstring>
#include <memory>
#include <cctype>
#include <set>

#include "types.h"

Element::Element()
    : c(0)
{ }

Element::Element(char c)
    : c(c)
{ }

Element::operator char() const
{
    return c;
}

bool Element::is_terminal() const
{
    return c == '#' || islower(c);
}

bool CFG::match(string word)
{
    if (!simplified)
        simplify();
    if (word.length() == 0 && includes_empty_word)
        return true;
    set<char> characters(word.begin(), word.end());
    set<char> extra_characters;
    set_difference(characters.begin(), characters.end(), alphabet.begin(), alphabet.end(), inserter(extra_characters, extra_characters.begin()));
    if (!extra_characters.empty()) {
        // В слове есть символы отсутствующие в грамматике
        return false;
    }

    CFGMatcher matcher(*this, word);
    return matcher.matches();
}

vector<reference_wrapper<Rule>> CFG::rule(Variable name)
{
    auto matching_rules = vector<reference_wrapper<Rule>>();
    for (auto &r: rules) {
        if (r.name == name) {
            matching_rules.push_back(r);
        }
    }
    return matching_rules;
}

set<Variable> CFG::variables()
{
    set<Variable> vars;
    for (const auto &r: rules) {
        vars.insert(r.name);
    }
    return move(vars);
}

void CFG::print()
{
    cout << "Алфавит: ";
    for (char c: alphabet) {
        cout << c << " ";
    }
    cout << endl;

    cout << "Переменные: ";
    for (char c: variables()) {
        cout << c << " ";
    }
    cout << endl;

    cout << "Начальное состояние: " << start << endl;

    cout << "Правила: " << endl;
    for (const Rule &r: rules) {
        cout << r.name << " -> ";
        for (Element e: r.production) {
            cout << e;
        }
        cout << endl;
    }
}

unique_ptr<CFG> CFG::from_mu_expression(string mu_expression)
{
    MuParser parser(mu_expression);
    auto cfg = parser.getCFG();
    cfg->sort_rules();
    return cfg;
}

void CFG::simplify()
{
    remove_epsilon_productions();
    int count;
    do {
        count = rules.size();
        remove_unit_productions();
    } while (rules.size() != count);
    simplified = true;
}

void CFG::remove_epsilon_productions()
{
    vector<Variable> epsilon_vars;
    vector<Rule> next_rules;
    for (const auto &rule: rules)
        if (rule.production.size() == 1 && rule.production[0] == '#') {
            epsilon_vars.push_back(rule.name);
            if (rule.name == start)
               includes_empty_word = true;
        }
        else
            next_rules.push_back(rule);
    rules.assign(next_rules.begin(), next_rules.end());

    next_rules.clear();
    for (const auto &rule: rules) {
        auto new_rules = make_epsilonless_rule(rule, 0, epsilon_vars);
        next_rules.insert(next_rules.end(), new_rules.begin(), new_rules.end());
    }
    rules.assign(next_rules.begin(), next_rules.end());
    sort_rules();
    remove_duplicates();
}

void CFG::remove_unit_productions()
{
    vector<Rule> to_remove;
    for (const auto &rule: rules)
        if (rule.production.size() == 1 && !rule.production[0].is_terminal())
            to_remove.push_back(rule);
    for (const auto &rule: to_remove)
        remove_unit_production(rule);
    for (const auto &r: to_remove)
        rules.erase(remove(rules.begin(), rules.end(), r), rules.end());
    sort_rules();
    remove_duplicates();
}

vector<Rule> CFG::make_epsilonless_rule(Rule rule, int pos, vector<Variable> epsilon_vars)
{
    if (rule.production.size() <= pos)
        return {rule};
    vector<Rule> rules = make_epsilonless_rule(rule, pos + 1, epsilon_vars);
    if (find(epsilon_vars.begin(), epsilon_vars.end(), rule.production[pos]) != epsilon_vars.end()) {
        auto new_rule = rule;
        new_rule.production.erase(new_rule.production.begin() + pos);
        if (new_rule.production.size() > 0) {
            auto new_rules = make_epsilonless_rule(new_rule, pos, epsilon_vars);
            rules.insert(rules.end(), new_rules.begin(), new_rules.end());
        } else if (new_rule.name == start) {
            includes_empty_word = true;
        }
    }
    return move(rules);
}

void CFG::remove_unit_production(Rule rule)
{
    vector<Rule> extra_rules;
    Variable replace_with = rule.production[0];
    for (const auto &r: rules) {
        if (r.name == replace_with) {
            Rule new_rule;
            new_rule.name = rule.name;
            new_rule.production = r.production;
            extra_rules.push_back(new_rule);
        }
    }
    for (const auto &r: extra_rules) {
        rules.push_back(r);
    }
}

void CFG::remove_duplicates()
{
    rules.erase(unique(rules.begin(), rules.end()), rules.end());
}

void CFG::sort_rules()
{
    sort(rules.begin(), rules.end(), [](auto a, auto b) {
        if (a.name == b.name) {
            return string(a.production.begin(), a.production.end()) <
                   string(b.production.begin(), b.production.end());
        }
        return a.name < b.name;
    });
}

MuParser::MuParser(string expression)
    : expression(expression)
    , pos(0)
    , next_nonterminal('A')
{ }

unique_ptr<CFG> MuParser::getCFG()
{
    if (!cfg)
    {
        cfg = make_unique<CFG>();
        parse_expression();
    }
    return move(cfg);
}

void MuParser::parse_expression()
{
    Variable start = parse_mu_expr();
    cfg->start = start;
}

Variable MuParser::parse_mu_expr()
{
    expect("mu(");
    Terminal v = next();
    Variable rule_name = next_nonterminal++;
    mu_parameters[v] = rule_name;
    expect(").");
    parse_rule(rule_name);
    return rule_name;
}

void MuParser::parse_rule(Variable rule_name)
{
    auto r = make_unique<Rule>(rule_name);
    for(;;) {
        char c = next();
        switch(c) {
        case '\0':
        case ')':
            goto done;
        case ' ':
            break;
        case '(':
            r->production.push_back(parse_mu_expr());
            break;
        case '+':
            cfg->rules.push_back(*r);
            r = make_unique<Rule>(rule_name);
            break;
        default:
            if (islower(c) && is_mu_parameter(c))
                cfg->alphabet.insert(c);
            auto rule_name = mu_parameters.find(c);
            if (rule_name == mu_parameters.end())
                r->production.push_back(c);
            else
                r->production.push_back(rule_name->second);
        }
    }
    done:
    cfg->rules.push_back(*r);
}

void MuParser::expect(string s)
{
    for (auto c: s) {
        if (next() != c) {
            ostringstream error_str;
            error_str << "Invalid character at position " << pos << ". Expected: " << s;
            throw runtime_error(error_str.str());
        }
    }
}

char MuParser::next()
{
    return expression[pos++];
}

bool MuParser::is_mu_parameter(Terminal t)
{
    for (auto kv: mu_parameters) {
        if (t == kv.first)
            return false;
    }
    return true;
}

CFGMatcher::CFGMatcher(CFG cfg, string word)
    : cfg(move(cfg))
    , word(move(word))
    , pos(0)
{ }

bool CFGMatcher::matches()
{
    if (result.has_value())
        return result.value();

    init();
    for (const auto &r: cfg.rule(cfg.start)) {
        add(MatchState(r, 0, 0), 0);
    }
    for (int k = 0; k < word.length() + 1; k++) {
        for (auto &state: states[k]) {
            match_iteration(state, k);
        }
    }
    // Проверяем смогли ли спарсить всё слово
    for (const auto &s: states[word.length()]) {
        if (s.rule.name == cfg.start && s.origin == 0 && s.pos == s.rule.production.size()) {
            result = true;
            return result.value();
        }
    }
    result = false;
    return result.value();
}

void CFGMatcher::init()
{
    states.resize(word.length() + 1);
}

void CFGMatcher::match_iteration(MatchState state, int k)
{
    if (state.is_finished())
        complete(state, k);
    else if (state.next().is_terminal())
        scan(state, k);
    else
        predict(state, k);
}

void CFGMatcher::add(MatchState state, int k)
{
    auto result = states[k].insert(move(state));
    if (result.second == true)
        match_iteration(*(result.first), k);
}

void CFGMatcher::predict(MatchState state, int k)
{
    for (const Rule &r: cfg.rule(state.next())) {
        add(MatchState(r, 0, k), k);
    }
}

void CFGMatcher::scan(MatchState state, int k)
{
    char next = state.next();
    if (next == '#')
        add(MatchState(state.rule, state.pos + 1, state.origin), k);
    else if (next == word[k])
        add(MatchState(state.rule, state.pos + 1, state.origin), k + 1);
}

void CFGMatcher::complete(MatchState state, int k)
{
    for (auto &s: states[state.origin]) {
        if (state.rule.name == s.next()) {
            add(MatchState(s.rule, s.pos + 1, s.origin), k);
        }
    }
}

Rule::Rule()
{ }

Rule::Rule(Variable name)
    : name(name)
{ }

bool Rule::operator ==(const Rule &other) const
{
    if (name != other.name || production.size() != other.production.size())
        return false;
    for (int i = 0; i < production.size(); i++) {
        if (production[i] != other.production[i])
            return false;
    }
    return true;
}

CFGMatcher::MatchState::MatchState(Rule rule, int pos, int origin)
    : rule(move(rule))
    , pos(pos)
    , origin(origin)
{ }

bool CFGMatcher::MatchState::operator ==(const MatchState &other) const
{
    return rule == other.rule && pos == other.pos && origin == other.origin;
}

Element CFGMatcher::MatchState::next() const
{
    return rule.production[pos];
}

bool CFGMatcher::MatchState::is_finished() const
{
    return rule.production.size() == pos;
}

size_t hash<CFGMatcher::MatchState>::operator()(const CFGMatcher::MatchState &k) const
{
    hash<Rule> rule_hash{};
    return (((k.origin << 3) ^ k.pos) << 1) ^ rule_hash(k.rule);
}

size_t hash<Rule>::operator()(const Rule &k) const
{
    size_t result = hash<char>{}(k.name);
    for (const auto &el: k.production) {
        result <<= 1;
        result ^= el;
    }
    return result;
}
