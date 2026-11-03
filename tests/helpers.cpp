#include "helpers.h"
#include <common/standard.h>
#include <arithmetic/algorithm.h>

namespace test {

vector<petri::iterator> findRule(const chp::graph &g, arithmetic::Expression guard, arithmetic::Choice action) {
	guard.top = arithmetic::minimize(guard, {guard.top}).map(guard.top);
	for (auto i = action.terms.begin(); i != action.terms.end(); i++) {
		for (auto j = i->actions.begin(); j != i->actions.end(); j++) {
			j->lvalue.minimize();
			j->rvalue.minimize();
		}
	}

	vector<petri::iterator> result;
	for (int i = 0; i < (int)g.transitions.size(); i++) {
		if (not g.transitions.is_valid(i)) continue;

		arithmetic::Expression e = g.transitions[i].guard;
		e.top = arithmetic::minimize(e, {e.top}).map(e.top);
		arithmetic::Choice c = g.transitions[i].action;
		for (auto j = c.terms.begin(); j != c.terms.end(); j++) {
			for (auto k = j->actions.begin(); k != j->actions.end(); k++) {
				k->lvalue.minimize();
				k->rvalue.minimize();
			}
		}
		if (areSame(e, guard) and areSame(c, action)) {
			result.push_back(petri::iterator(petri::transition::type, i));
		}
	}
	return result;
}

// Helper function to parse a string into an HSE graph via COG
chp::graph load_cog_string(string input) {
	// Set up tokenizer
	tokenizer tokens;
	tokens.register_token<parse::block_comment>(false);
	tokens.register_token<parse::line_comment>(false);
	parse_cog::factory.register_syntax(tokens);
	
	tokens.insert("string_input", input, nullptr);
	
	chp::graph g;

	// Parse input
	tokens.increment(false);
	tokens.expect<parse_cog::composition>();
	if (tokens.decrement(__FILE__, __LINE__)) {
		parse_cog::composition syntax(tokens);
		chp::import_chp(g, syntax, &tokens, true);
	}

	g.post_process();

	return g;
}

}
