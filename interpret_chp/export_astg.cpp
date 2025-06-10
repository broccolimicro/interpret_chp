#include "export_astg.h"
#include <interpret_arithmetic/export.h>

namespace chp {

pair<parse_astg::node, parse_astg::node> export_astg(parse_astg::graph &astg, const chp::graph &g, chp::iterator pos, map<chp::iterator, pair<parse_astg::node, parse_astg::node> > &nodes, string tlabel, string plabel)
{
	map<chp::iterator, pair<parse_astg::node, parse_astg::node> >::iterator loc = nodes.find(pos);
	if (loc == nodes.end()) {
		if (pos.type == chp::transition::type) {
			pair<parse_astg::node, parse_astg::node> inout;

			parse_expression::expression guard = export_expression(g.transitions[pos.index].guard, g);
			parse_expression::composition action = export_composition(g.transitions[pos.index].action, g);
			inout.first = parse_astg::node(guard, action, tlabel);
			inout.second = inout.first;

			loc = nodes.insert(pair<chp::iterator, pair<parse_astg::node, parse_astg::node> >(pos, inout)).first;
		} else {
			pair<parse_astg::node, parse_astg::node> inout;
			inout.first = parse_astg::node("p" + plabel);
			inout.second = inout.first;

			loc = nodes.insert(pair<chp::iterator, pair<parse_astg::node, parse_astg::node> >(pos, inout)).first;
		}
	}

	return loc->second;
}

parse_astg::graph export_astg(const chp::graph &g)
{
	parse_astg::graph result;

	result.name = "chp";

	// Add the variables
	for (int i = 0; i < (int)g.vars.size(); i++)
		result.internal.push_back(ucs::Net(g.netAt(i)));

	// Add the arcs
	map<chp::iterator, pair<parse_astg::node, parse_astg::node> > nodes;
	vector<int> forks;
	for (int i = 0; i < (int)g.transitions.size(); i++)
	{
		int curr = result.arcs.size();
		result.arcs.push_back(parse_astg::arc());
		pair<parse_astg::node, parse_astg::node> t0 = export_astg(result, g, chp::iterator(chp::transition::type, i), nodes, to_string(i), to_string(i));
		result.arcs[curr].nodes.push_back(t0.second);

		vector<int> n = g.next(chp::transition::type, i);


		for (int j = 0; j < (int)n.size(); j++)
		{
			vector<int> nn = g.next(chp::place::type, n[j]);
			vector<int> pn = g.prev(chp::place::type, n[j]);

			bool is_reset = false;
			for (int k = 0; k < (int)g.reset.size() && !is_reset; k++)
				for (int l = 0; l < (int)g.reset[k].tokens.size() && !is_reset; l++)
					if (n[j] == g.reset[k].tokens[l].index)
						is_reset = true;

			pair<parse_astg::node, parse_astg::node> p1 = export_astg(result, g, chp::iterator(chp::place::type, n[j]), nodes, to_string(n[j]), to_string(n[j]));
			result.arcs[curr].nodes.push_back(p1.second);
			forks.push_back(n[j]);
		}
	}

	sort(forks.begin(), forks.end());
	forks.resize(unique(forks.begin(), forks.end()) - forks.begin());

	for (int i = 0; i < (int)forks.size(); i++)
	{
		int curr = result.arcs.size();
		result.arcs.push_back(parse_astg::arc());
		pair<parse_astg::node, parse_astg::node> p0 = export_astg(result, g, chp::iterator(chp::place::type, forks[i]), nodes, to_string(forks[i]), to_string(forks[i]));
		result.arcs[curr].nodes.push_back(p0.second);

		vector<int> n = g.next(chp::place::type, forks[i]);

		for (int j = 0; j < (int)n.size(); j++)
		{
			pair<parse_astg::node, parse_astg::node> t1 = export_astg(result, g, chp::iterator(chp::transition::type, n[j]), nodes, to_string(n[j]), to_string(n[j]));
			result.arcs[curr].nodes.push_back(t1.second);
		}
	}

	// Add the initial markings
	for (int i = 0; i < (int)g.reset.size(); i++)
	{
		result.marking.push_back(pair<parse_expression::composition, vector<parse_astg::node> >());
		result.marking.back().first = export_composition(g.reset[i].encodings, g);
		for (int j = 0; j < (int)g.reset[i].tokens.size(); j++)
			result.marking.back().second.push_back(parse_astg::node("p" + to_string(g.reset[i].tokens[j].index)));
	}

	// Add the arbiters
	for (int i = 0; i < (int)g.places.size(); i++) {
		if (g.places[i].arbiter) {
			result.arbiter.push_back(parse_astg::node("p" + to_string(i)));
		}
	}

	return result;
}

}
