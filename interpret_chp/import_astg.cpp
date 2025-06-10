#include "import_astg.h"
#include <interpret_arithmetic/import.h>

namespace chp {

chp::iterator import_chp(const parse_astg::node &syntax, chp::graph &g, map<string, chp::iterator> &ids, tokenizer *tokens)
{
	chp::iterator i(-1,-1);
	if (syntax.id.size() > 0) {
		i.type = chp::transition::type;
		i.index = std::stoi(syntax.id);
	} else if (syntax.place.size() > 0 && syntax.place[0] == 'p') {
		i.type = chp::place::type;
		i.index = std::stoi(syntax.place.substr(1));
	} else if (tokens != NULL) {
		tokens->load(&syntax);
		tokens->error("Undefined node", __FILE__, __LINE__);
		return i;
	} else {
		error("", "Undefined node \"" + syntax.to_string() + "\"", __FILE__, __LINE__);
		return i;
	}
	
	auto created = ids.insert(pair<string, chp::iterator>(syntax.to_string(), i));
	if (created.second && i.type == chp::transition::type)
	{
		arithmetic::Expression guard(true);
		arithmetic::Choice action;
		if (syntax.guard.valid) {
			guard = arithmetic::import_expression(syntax.guard, g, 0, tokens, false);
		}
		if (syntax.assign.valid) {
			action = arithmetic::import_choice(syntax.assign, g, 0, tokens, false);
		} else {
			action.terms.push_back(arithmetic::Parallel());
		}

		g.create_at(chp::transition(guard, action), i.index);
	} else if (created.second) {
		g.create_at(chp::place(), i.index);
	}

	return i;
}

void import_chp(const parse_astg::arc &syntax, chp::graph &g, map<string, chp::iterator> &ids, tokenizer *tokens)
{
	chp::iterator base = import_chp(syntax.nodes[0], g, ids, tokens);
	for (int i = 1; i < (int)syntax.nodes.size(); i++)
	{
		chp::iterator next = import_chp(syntax.nodes[i], g, ids, tokens);
		g.connect(base, next);
	}
}

chp::graph import_chp(const parse_astg::graph &syntax, tokenizer *tokens)
{
	chp::graph result;
	map<string, chp::iterator> ids;
	for (int i = 0; i < (int)syntax.inputs.size(); i++)
		arithmetic::import_net(syntax.inputs[i].to_string(), result, tokens, true);

	for (int i = 0; i < (int)syntax.outputs.size(); i++)
		arithmetic::import_net(syntax.outputs[i].to_string(), result, tokens, true);

	for (int i = 0; i < (int)syntax.internal.size(); i++)
		arithmetic::import_net(syntax.internal[i].to_string(), result, tokens, true);

	for (int i = 0; i < (int)syntax.arcs.size(); i++)
		import_chp(syntax.arcs[i], result, ids, tokens);

	for (int i = 0; i < (int)syntax.marking.size(); i++)
	{
		chp::state rst;
		if (syntax.marking[i].first.valid)
			rst.encodings = arithmetic::import_state(syntax.marking[i].first, result, 0, tokens, false);

		for (int j = 0; j < (int)syntax.marking[i].second.size(); j++)
		{
			chp::iterator loc = import_chp(syntax.marking[i].second[j], result, ids, tokens);
			if (loc.type == chp::place::type && loc.index >= 0)
				rst.tokens.push_back(loc.index);
		}
		result.reset.push_back(rst);
	}

	for (int i = 0; i < (int)syntax.arbiter.size(); i++) {
		chp::iterator loc = import_chp(syntax.arbiter[i], result, ids, tokens);
		if (loc.type == chp::place::type and loc.index >= 0) {
			result.places[loc.index].arbiter = true;
		}
	}

	return result;
}

}
