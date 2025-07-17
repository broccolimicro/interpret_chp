#include "helpers.h"
#include <common/standard.h>
#include <arithmetic/algorithm.h>

namespace test {

vector<petri::iterator> findRule(const chp::graph &g, arithmetic::Expression guard, arithmetic::Choice action) {
	guard.top = arithmetic::minimize(guard, {guard.top}).map(guard.top);
	for (auto i = action.terms.begin(); i != action.terms.end(); i++) {
		for (auto j = i->actions.begin(); j != i->actions.end(); j++) {
			j->expr.top = arithmetic::minimize(j->expr, {j->expr.top}).map(j->expr.top);
		}
	}

	vector<petri::iterator> result;
	for (int i = 0; i < (int)g.transitions.size(); i++) {
		arithmetic::Expression e = g.transitions[i].guard;
		e.top = arithmetic::minimize(e, {e.top}).map(e.top);
		arithmetic::Choice c = g.transitions[i].action;
		for (auto j = c.terms.begin(); j != c.terms.end(); j++) {
			for (auto k = j->actions.begin(); k != j->actions.end(); k++) {
				k->expr.top = arithmetic::minimize(k->expr, {k->expr.top}).map(k->expr.top);
			}
		}
		if (areSame(e, guard) and areSame(c, action)) {
			result.push_back(petri::iterator(petri::transition::type, i));
		}
	}
	return result;
}

}
