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
		if (areSame(g.transitions[i].guard, guard) and areSame(g.transitions[i].action, action)) {
			result.push_back(petri::iterator(petri::transition::type, i));
		}
	}
	return result;
}

}
