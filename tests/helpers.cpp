#include "helpers.h"
#include <common/standard.h>

namespace test {

vector<petri::iterator> findRule(const chp::graph &g, arithmetic::Expression guard, arithmetic::Choice action) {
	guard = guard.minimize();
	for (auto i = action.terms.begin(); i != action.terms.end(); i++) {
		for (auto j = i->actions.begin(); j != i->actions.end(); j++) {
			j->expr = j->expr.minimize();
		}
	}

	vector<petri::iterator> result;
	for (int i = 0; i < (int)g.transitions.size(); i++) {
		arithmetic::Expression e = g.transitions[i].guard;
		e.minimize();
		arithmetic::Choice c = g.transitions[i].action;
		for (auto j = c.terms.begin(); j != c.terms.end(); j++) {
			for (auto k = j->actions.begin(); k != j->actions.end(); k++) {
				k->expr = k->expr.minimize();
			}
		}
		if (areSame(e, guard) and areSame(c, action)) {
			result.push_back(petri::iterator(petri::transition::type, i));
		}
	}
	return result;
}

}
