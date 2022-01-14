#include <functional>
#include <iostream>
#include <string>
#include <regex>
#include <cassert>

#include "types.h"

using namespace std;

void die(string msg) {
    cout << msg << endl;
    exit(1);
}

void with_test(string expr, const function<void (const CFG&)> &func) {
    auto cfg = CFG::from_mu_expression(expr);
    cout << expr << endl;
    cfg->print();
    cout << endl;
    func(*cfg);
}

void test() {
    with_test("mu(x).xbx + a + #", [](auto cfg) {
        assert(true == cfg.match(""));
        assert(true == cfg.match("a"));
        assert(true == cfg.match("b"));
        assert(true == cfg.match("ab"));
        assert(true == cfg.match("ba"));
        assert(true == cfg.match("aba"));
        assert(true == cfg.match("bab"));
        assert(false == cfg.match("c"));
        assert(false == cfg.match("aa"));
    });
    with_test("mu(x).ab(mu(y).cxa) + #", [](auto cfg) {
        assert(true == cfg.match(""));
        assert(true == cfg.match("abca"));
        assert(true == cfg.match("abcabcaa"));
        assert(true == cfg.match("abcabcabcaaa"));
        assert(false == cfg.match("ab"));
        assert(false == cfg.match("abc"));
    });
    with_test("mu(s).(mu(x).ax+#)(mu(x).bx+#)", [](auto cfg) {
        assert(true == cfg.match(""));
        assert(true == cfg.match("a"));
        assert(true == cfg.match("aa"));
        assert(true == cfg.match("b"));
        assert(true == cfg.match("bb"));
        assert(true == cfg.match("ab"));
        assert(false == cfg.match("ba"));
    });
    with_test("mu(X).XX + (mu(Y).YY + (mu(Z).ZZ + a + #))", [](auto cfg) {
        cout << "Убираем правила вида X -> #\n";
        cfg.remove_epsilon_productions();
        cfg.print();
        cout << "\nУбираем правила вида X -> Y\n";
        cfg.remove_unit_productions();
        cfg.remove_unit_productions();
        cfg.print();
        assert(true == cfg.match("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    });
}

void input()
{
    string input;
    // Терминалы и нетерминалы спрашивать бесполезно, т.к. если их нет в mu-выражении, то их нет и в грамматике.
    // Даже если они есть в алфавите языка, они всё равно не могут использоваться ни в одном слове.
    // Поэтому читаем mu-выражение и берём терминалы и нетерминалы из него.
    cout << "Введите mu-выражение. Пример: mu(x).xbx + a + #\n";
    getline(cin, input);
    auto cfg = CFG::from_mu_expression(input);
    cout << endl;
    cfg->print();
    cout << endl;
    for (;;) {
        string word;
        cout << "Введите слово (ctrl+d чтобы завершить):" << endl;
        cin >> word;
        if (word == "")
            exit(0);
        if (cfg->match(word)) {
            cout << "Слово " << word << " является словом в этом языке." << endl;
        } else {
            cout << "Слово " << word << " НЕ является словом в этом языке." << endl;
        }
    }
}

int main() {
    test();
    input();
    return 0;
}
