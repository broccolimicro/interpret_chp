/*
 * import.cpp
 *
 *  Created on: Feb 6, 2015
 *      Author: nbingham
 */

#include "import.h"
#include <interpret_arithmetic/import.h>

namespace chp {

chp::iterator import_chp(const parse_astg::node &syntax, ucs::variable_set &variables, chp::graph &g, map<string, chp::iterator> &ids, tokenizer *tokens)
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
		arithmetic::expression guard;
		arithmetic::choice action;
		if (syntax.guard.valid) {
			guard = import_expression(syntax.guard, variables, 0, tokens, false);
		}
		if (syntax.assign.valid) {
			action = import_choice(syntax.assign, variables, 0, tokens, false);
		}
		
		g.create_at(chp::transition(guard, action), i.index);
	} else if (created.second) {
		g.create_at(chp::place(), i.index);
	}

	return i;
}

void import_chp(const parse_astg::arc &syntax, ucs::variable_set &variables, chp::graph &g, map<string, chp::iterator> &ids, tokenizer *tokens)
{
	chp::iterator base = import_chp(syntax.nodes[0], variables, g, ids, tokens);
	for (int i = 1; i < (int)syntax.nodes.size(); i++)
	{
		chp::iterator next = import_chp(syntax.nodes[i], variables, g, ids, tokens);
		g.connect(base, next);
	}
}

chp::graph import_chp(const parse_astg::graph &syntax, ucs::variable_set &variables, tokenizer *tokens)
{
	chp::graph result;
	map<string, chp::iterator> ids;
	for (int i = 0; i < (int)syntax.inputs.size(); i++)
		define_variables(syntax.inputs[i], variables, 0, tokens, true, false);

	for (int i = 0; i < (int)syntax.outputs.size(); i++)
		define_variables(syntax.outputs[i], variables, 0, tokens, true, false);

	for (int i = 0; i < (int)syntax.internal.size(); i++)
		define_variables(syntax.internal[i], variables, 0, tokens, true, false);

	for (int i = 0; i < (int)syntax.arcs.size(); i++)
		import_chp(syntax.arcs[i], variables, result, ids, tokens);

	for (int i = 0; i < (int)syntax.marking.size(); i++)
	{
		chp::state rst;
		if (syntax.marking[i].first.valid)
			rst.encodings = import_state(syntax.marking[i].first, variables, 0, tokens, false);

		for (int j = 0; j < (int)syntax.marking[i].second.size(); j++)
		{
			chp::iterator loc = import_chp(syntax.marking[i].second[j], variables, result, ids, tokens);
			if (loc.type == chp::place::type && loc.index >= 0)
				rst.tokens.push_back(loc.index);
		}
		result.reset.push_back(rst);
	}

	for (int i = 0; i < (int)syntax.arbiter.size(); i++) {
		chp::iterator loc = import_chp(syntax.arbiter[i], variables, result, ids, tokens);
		if (loc.type == chp::place::type and loc.index >= 0) {
			result.places[loc.index].arbiter = true;
		}
	}

	return result;
}


petri::iterator import_chp(const parse_dot::node_id &syntax, map<string, petri::iterator> &nodes, ucs::variable_set &variables, chp::graph &g, tokenizer *tokens, bool define, bool squash_errors)
{
	if (syntax.valid && syntax.id.size() > 0)
	{
		map<string, petri::iterator>::iterator i = nodes.find(syntax.id);
		if (i != nodes.end())
		{
			if (define && !squash_errors)
			{
				if (tokens != NULL)
				{
					tokens->load(&syntax);
					tokens->error("redefinition of node '" + syntax.id + "'", __FILE__, __LINE__);
					if (tokens->load(syntax.id))
						tokens->note("previously defined here", __FILE__, __LINE__);
				}
				else
					error(syntax.to_string(), "redefinition of node '" + syntax.id + "'", __FILE__, __LINE__);
			}
			return i->second;
		}
		else
		{

			if (!define && !squash_errors)
			{
				if (tokens != NULL)
				{
					tokens->load(&syntax);
					tokens->error("node '" + syntax.id + "' not yet defined", __FILE__, __LINE__);
				}
				else
					error(syntax.to_string(), "node '" + syntax.id + "' not yet defined", __FILE__, __LINE__);
			}
			else if (tokens != NULL)
				tokens->save(syntax.id, &syntax);

			if (syntax.id[0] == 'P')
			{
				petri::iterator result = g.create(chp::place());
				nodes.insert(pair<string, petri::iterator>(syntax.id, result));
				return result;
			}
			else if (syntax.id[0] == 'T')
			{
				petri::iterator result = g.create(chp::transition());
				nodes.insert(pair<string, petri::iterator>(syntax.id, result));
				return result;
			}
			else
			{
				if (tokens != NULL)
				{
					tokens->load(&syntax);
					tokens->error("Unrecognized node type '" + syntax.id + "'", __FILE__, __LINE__);
				}
				else
					error(syntax.to_string(), "Unrecognized node type '" + syntax.id + "'", __FILE__, __LINE__);
				return petri::iterator();
			}
		}
	}
	else
		return petri::iterator();
}

map<string, string> import_chp(const parse_dot::attribute_list &syntax, tokenizer *tokens)
{
	map<string, string> result;
	if (syntax.valid)
		for (vector<parse_dot::assignment_list>::const_iterator l = syntax.attributes.begin(); l != syntax.attributes.end(); l++)
			if (l->valid)
				for (vector<parse_dot::assignment>::const_iterator a = l->as.begin(); a != l->as.end(); a++)
					result.insert(pair<string, string>(a->first, a->second));

	return result;
}

void import_chp(const parse_dot::statement &syntax, chp::graph &g, ucs::variable_set &variables, map<string, map<string, string> > &globals, map<string, petri::iterator> &nodes, tokenizer *tokens, bool auto_define)
{
	map<string, string> attributes = import_chp(syntax.attributes, tokens);
	map<string, string>::iterator attr;

	if (syntax.statement_type == "attribute")
	{
		for (attr = attributes.begin(); attr != attributes.end(); attr++)
			globals[syntax.attribute_type][attr->first] = attr->second;
	}
	else
	{
		vector<petri::iterator> p;
		for (int i = 0; i < (int)syntax.nodes.size(); i++)
		{
			vector<petri::iterator> n;
			if (syntax.nodes[i]->is_a<parse_dot::node_id>())
				n.push_back(import_chp(*(parse_dot::node_id*)syntax.nodes[i], nodes, variables, g, tokens, (syntax.statement_type == "node"), auto_define));
			else if (syntax.nodes[i]->is_a<parse_dot::graph>())
			{
				map<string, map<string, string> > sub_globals = globals;
				for (attr = attributes.begin(); attr != attributes.end(); attr++)
					sub_globals[syntax.statement_type][attr->first] = attr->second;
				import_chp(*(parse_dot::graph*)syntax.nodes[i], g, variables, sub_globals, nodes, tokens, auto_define);
			}

			attr = attributes.find("label");
			if (attr == attributes.end())
				attr = globals[syntax.statement_type].find("label");

			arithmetic::choice c;
			if (attr != attributes.end() && attr != globals[syntax.statement_type].end() && attr->second.size() != 0)
			{
				tokenizer temp;
				parse_expression::composition::register_syntax(temp);
				temp.insert(attr->first, attr->second);

				temp.increment(true);
				temp.expect<parse_expression::composition>();

				if (temp.decrement(__FILE__, __LINE__))
				{
					parse_expression::composition exp(temp);
					c = import_choice(exp, variables, 0, &temp, true);
				}

				for (int i = 0; i < (int)n.size(); i++)
					if (n[i].type == chp::transition::type)
						g.transitions[n[i].index].local_action = c;
			}

			if (i != 0)
				g.connect(p, n);

			p = n;
		}
	}
}

void import_chp(const parse_dot::graph &syntax, chp::graph &g, ucs::variable_set &variables, map<string, map<string, string> > &globals, map<string, petri::iterator> &nodes, tokenizer *tokens, bool auto_define)
{
	if (syntax.valid)
		for (int i = 0; i < (int)syntax.statements.size(); i++)
			import_chp(syntax.statements[i], g, variables, globals, nodes, tokens, auto_define);
}

chp::graph import_chp(const parse_dot::graph &syntax, ucs::variable_set &variables, tokenizer *tokens, bool auto_define)
{
	chp::graph result;
	map<string, map<string, string> > globals;
	map<string, petri::iterator> nodes;
	if (syntax.valid)
		for (int i = 0; i < (int)syntax.statements.size(); i++)
			import_chp(syntax.statements[i], result, variables, globals, nodes, tokens, auto_define);
	return result;
}

chp::graph import_chp(const parse_expression::expression &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define)
{
	chp::graph result;
	petri::iterator b = result.create(chp::place());
	petri::iterator t = result.create(chp::transition(import_expression(syntax, variables, default_id, tokens, auto_define), arithmetic::choice()));
	petri::iterator e = result.create(chp::place());

	result.connect(b, t);
	result.connect(t, e);

	result.source.push_back(chp::state(vector<chp::token>(1, chp::token(b.index)), arithmetic::state()));
	result.sink.push_back(chp::state(vector<chp::token>(1, chp::token(e.index)), arithmetic::state()));
	return result;
}

chp::graph import_chp(const parse_expression::assignment &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define)
{
	chp::graph result;
	petri::iterator b = result.create(chp::place());
	petri::iterator t = result.create(chp::transition(arithmetic::expression(), import_action(syntax, variables, default_id, tokens, auto_define)));
	petri::iterator e = result.create(chp::place());

	result.connect(b, t);
	result.connect(t, e);

	result.source.push_back(chp::state(vector<chp::token>(1, chp::token(b.index)), arithmetic::state()));
	result.sink.push_back(chp::state(vector<chp::token>(1, chp::token(e.index)), arithmetic::state()));
	return result;
}

chp::graph import_chp(const parse_chp::composition &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define)
{
	if (syntax.region != "")
		default_id = atoi(syntax.region.c_str());

	chp::graph result;

	int composition = petri::parallel;
	if (parse_chp::composition::precedence[syntax.level] == "||" || parse_chp::composition::precedence[syntax.level] == ",")
		composition = petri::parallel;
	else if (parse_chp::composition::precedence[syntax.level] == ";")
		composition = petri::sequence;

	for (int i = 0; i < (int)syntax.branches.size(); i++)
	{
		if (syntax.branches[i].sub.valid)
			result.merge(composition, import_chp(syntax.branches[i].sub, variables, default_id, tokens, auto_define));
		else if (syntax.branches[i].ctrl.valid)
			result.merge(composition, import_chp(syntax.branches[i].ctrl, variables, default_id, tokens, auto_define));
		else if (syntax.branches[i].assign.valid)
			result.merge(composition, import_chp(syntax.branches[i].assign, variables, default_id, tokens, auto_define));

		if (syntax.reset == 0 && i == 0)
			result.reset = result.source;
		else if (syntax.reset == i+1)
			result.reset = result.sink;
	}

	if (syntax.branches.size() == 0)
	{
		petri::iterator b = result.create(chp::place());

		result.source.push_back(chp::state(vector<chp::token>(1, chp::token(b.index)), arithmetic::state()));
		result.sink.push_back(chp::state(vector<chp::token>(1, chp::token(b.index)), arithmetic::state()));

		if (syntax.reset >= 0)
			result.reset = result.source;
	}

	return result;
}

chp::graph import_chp(const parse_chp::control &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define)
{
	if (syntax.region != "")
		default_id = atoi(syntax.region.c_str());

	chp::graph result;

	for (int i = 0; i < (int)syntax.branches.size(); i++)
	{
		chp::graph branch;
		if (syntax.branches[i].first.valid)
			branch.merge(petri::sequence, import_chp(syntax.branches[i].first, variables, default_id, tokens, auto_define));
		if (syntax.branches[i].second.valid)
			branch.merge(petri::sequence, import_chp(syntax.branches[i].second, variables, default_id, tokens, auto_define));
		result.merge(petri::choice, branch);
	}

	if (syntax.repeat)
	{
		arithmetic::expression repeat = arithmetic::operand(0);
		for (int i = 0; i < (int)syntax.branches.size(); i++)
		{
			if (syntax.branches[i].first.valid)
			{
				if (i == 0)
					repeat = !import_expression(syntax.branches[i].first, variables, default_id, tokens, auto_define);
				else
					repeat = repeat && !import_expression(syntax.branches[i].first, variables, default_id, tokens, auto_define);
			}
			else
			{
				repeat = arithmetic::operand(0, arithmetic::operand::neutral);
				break;
			}
		}

		petri::iterator sm = result.create(chp::place());
		for (int i = 0; i < (int)result.source.size(); i++)
		{
			if (result.source[i].tokens.size() > 1)
			{
				petri::iterator split_t = result.create(chp::transition());
				result.connect(sm, split_t);

				for (int j = 0; j < (int)result.source[i].tokens.size(); j++)
					result.connect(split_t, petri::iterator(chp::place::type, result.source[i].tokens[j].index));
			}
			else if (result.source[i].tokens.size() == 1)
			{
				petri::iterator loc(chp::place::type, result.source[i].tokens[0].index);
				result.connect(sm, result.next(loc));
				result.erase(loc);
				if (sm.index > loc.index)
					sm.index--;
			}
		}

		result.source.clear();

		for (int i = 0; i < (int)result.sink.size(); i++)
		{
			if (result.sink[i].tokens.size() > 1)
			{
				petri::iterator merge_t = result.create(chp::transition());
				result.connect(merge_t, sm);

				for (int j = 0; j < (int)result.sink[i].tokens.size(); j++)
					result.connect(petri::iterator(chp::place::type, result.sink[i].tokens[j].index), merge_t);
			}
			else if (result.sink[i].tokens.size() == 1)
			{
				petri::iterator loc(chp::place::type, result.sink[i].tokens[0].index);
				result.connect(result.prev(loc), sm);
				result.erase(loc);
				if (sm.index > loc.index)
					sm.index--;
			}
		}

		result.sink.clear();

		if (!repeat.is_constant() || repeat.evaluate(arithmetic::state()).data != arithmetic::value::neutral)
		{
			petri::iterator guard = result.create(chp::transition(repeat, arithmetic::choice()));
			result.connect(sm, guard);
			petri::iterator arrow = result.create(chp::place());
			result.connect(guard, arrow);

			result.sink.push_back(chp::state(vector<chp::token>(1, chp::token(arrow.index)), arithmetic::state()));
		}

		result.source.push_back(chp::state(vector<chp::token>(1, chp::token(sm.index)), arithmetic::state()));


		if (result.reset.size() > 0)
		{
			result.source = result.reset;
			result.reset.clear();
		}
	}

	return result;
}

}
