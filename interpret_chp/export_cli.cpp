#include "export_cli.h"

#include <interpret_arithmetic/export.h>

namespace chp {

string export_transition(const chp::graph &g, petri::iterator i, bool is_guard, bool here) {
	string result;

	bool render_guard = not g.transitions[i.index].guard.isValid();
	bool render_action = not g.transitions[i.index].action.isVacuous();

	if (is_guard) {
		if (render_guard) {
			result += arithmetic::export_expression(g.transitions[i.index].guard, g).to_string() + "->";
		} else {
			result += "vdd->";
		}
	} else {
		if (render_guard) {
			result += "[" + arithmetic::export_expression(g.transitions[i.index].guard, g).to_string() + "]";
		}

		if (render_guard and render_action) {
			result += ";";
		}
	}

	if (here) {
		result += "  <here>  ";
	}

	if (render_action) {
		result += arithmetic::export_composition(g.transitions[i.index].action, g).to_string();
	}

	if ((is_guard or not render_guard) and not render_action) {
		result += "skip";
	}
	return result;
}

string export_node(petri::iterator i, const chp::graph &g)
{
	vector<petri::iterator> n, p;
	if (i.type == chp::place::type) {
		p.push_back(i);
		n.push_back(i);
	} else {
		p = g.prev(i);
		n = g.next(i);
	}

	string result = "";

	if (p.size() > 1) {
		result += "(...";
	}
	for (int j = 0; j < (int)p.size(); j++)
	{
		if (j != 0)
			result += "||...";

		vector<petri::iterator> pp = g.prev(p[j]);
		if (pp.size() > 1) {
			result += "[...";
		}
		for (int j = 0; j < (int)pp.size(); j++) {
			if (j != 0)
				result += "[]...";

			result += export_transition(g, pp[j], false, false);
		}
		if (pp.size() > 1) {
			result += "]";
		}
	}
	if (p.size() > 1) {
		result += ")";
	}



	if (i.type == chp::place::type) {
		result += "; <here> ";
	} else {
		result += ";" + export_transition(g, i, false, true) + ";";
	}



	if (n.size() > 1) {
		result += "(";
	}
	for (int j = 0; j < (int)n.size(); j++)
	{
		if (j != 0)
			result += "...||";

		vector<petri::iterator> nn = g.next(n[j]);
		if (nn.size() > 1) {
			result += "[";
		}
		for (int j = 0; j < (int)nn.size(); j++) {
			if (j != 0)
				result += "...[]";


			result += export_transition(g, nn[j], nn.size() > 1, false);
		}
		if (nn.size() > 1) {
			result += "...]";
		}
	}
	if (n.size() > 1) {
		result += "...)";
	}

	return result;
}

}
