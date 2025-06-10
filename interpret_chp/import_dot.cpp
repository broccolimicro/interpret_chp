#include "import_dot.h"
#include <interpret_arithmetic/import.h>

namespace chp {

petri::iterator import_chp(const parse_dot::node_id &syntax, map<string, petri::iterator> &nodes, chp::graph &g, tokenizer *tokens, bool define, bool squash_errors)
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

void import_chp(const parse_dot::statement &syntax, chp::graph &g, map<string, map<string, string> > &globals, map<string, petri::iterator> &nodes, tokenizer *tokens, bool auto_define)
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
				n.push_back(import_chp(*(parse_dot::node_id*)syntax.nodes[i], nodes, g, tokens, (syntax.statement_type == "node"), auto_define));
			else if (syntax.nodes[i]->is_a<parse_dot::graph>())
			{
				map<string, map<string, string> > sub_globals = globals;
				for (attr = attributes.begin(); attr != attributes.end(); attr++)
					sub_globals[syntax.statement_type][attr->first] = attr->second;
				import_chp(*(parse_dot::graph*)syntax.nodes[i], g, sub_globals, nodes, tokens, auto_define);
			}

			attr = attributes.find("label");
			if (attr == attributes.end())
				attr = globals[syntax.statement_type].find("label");

			arithmetic::Choice c;
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
					c = arithmetic::import_choice(exp, g, 0, &temp, true);
				}

				for (int i = 0; i < (int)n.size(); i++)
					if (n[i].type == chp::transition::type)
						g.transitions[n[i].index].action = c;
			}

			if (i != 0)
				g.connect(p, n);

			p = n;
		}
	}
}

void import_chp(const parse_dot::graph &syntax, chp::graph &g, map<string, map<string, string> > &globals, map<string, petri::iterator> &nodes, tokenizer *tokens, bool auto_define)
{
	if (syntax.valid)
		for (int i = 0; i < (int)syntax.statements.size(); i++)
			import_chp(syntax.statements[i], g, globals, nodes, tokens, auto_define);
}

chp::graph import_chp(const parse_dot::graph &syntax, tokenizer *tokens, bool auto_define)
{
	chp::graph result;
	map<string, map<string, string> > globals;
	map<string, petri::iterator> nodes;
	if (syntax.valid)
		for (int i = 0; i < (int)syntax.statements.size(); i++)
			import_chp(syntax.statements[i], result, globals, nodes, tokens, auto_define);
	return result;
}

}
