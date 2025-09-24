#include "export_dot.h"

#include <interpret_arithmetic/export.h>

namespace chp {

parse_dot::node_id export_node_id(const chp::iterator &i)
{
	parse_dot::node_id result;
	result.valid = true;
	result.id = (i.type == chp::transition::type ? "T" : "P") + ::to_string(i.index);
	return result;
}

parse_dot::attribute_list export_attribute_list(const chp::iterator i, const chp::graph &g, bool labels, bool notations)
{
	parse_dot::attribute_list result;
	result.valid = true;
	parse_dot::assignment_list sub_result;
	sub_result.valid = true;

	if (i.type == chp::place::type) {
		parse_dot::assignment shape;
		shape.valid = true;
		shape.first = "shape";
		if (g.places[i.index].arbiter) {
			shape.second = "square";
		} else {
			shape.second = "circle";
		}
		sub_result.as.push_back(shape);

		bool is_reset = false;
		for (int j = 0; j < (int)g.reset.size() && !is_reset; j++) {
			for (int k = 0; k < (int)g.reset[j].tokens.size() && !is_reset; k++) {
				if (i.index == g.reset[j].tokens[k].index) {
					is_reset = true;
				}
			}
		}

		if (is_reset) {
			parse_dot::assignment marked;
			marked.valid = true;
			marked.first = "style";
			marked.second = "filled";

			sub_result.as.push_back(marked);
			if (!notations) {
				parse_dot::assignment color;
				color.valid = true;
				color.first = "fillcolor";
				color.second = "black";
				sub_result.as.push_back(color);

				parse_dot::assignment peripheries;
				peripheries.valid = true;
				peripheries.first = "peripheries";
				peripheries.second = "2";
				sub_result.as.push_back(peripheries);

				parse_dot::assignment size;
				size.valid = true;
				size.first = "width";
				size.second = "0.15";
				sub_result.as.push_back(size);
			}
		} else {
			parse_dot::assignment size;
			size.valid = true;
			size.first = "width";
			size.second = "0.18";
			sub_result.as.push_back(size);
		}

		parse_dot::assignment encoding;
		encoding.valid = true;
		encoding.first = "label";
		encoding.second = "";

		if (notations) {
			if (encoding.second != "") {
				encoding.second += "\n";
			}
			encoding.second += "[";
			for (int j = 0; j < (int)g.places[i.index].splits[petri::parallel].size(); j++) {
				if (j != 0) {
					encoding.second += ",";
				}
				encoding.second += g.places[i.index].splits[petri::parallel][j].to_string();
			}
			encoding.second += "]";
		}
		sub_result.as.push_back(encoding);
	} else {
		parse_dot::assignment plaintext;
		plaintext.valid = true;
		plaintext.first = "shape";
		plaintext.second = "plaintext";
		sub_result.as.push_back(plaintext);

		parse_dot::assignment action;
		action.valid = true;
		action.first = "label";
		bool g_vacuous = g.transitions[i.index].guard.isConstant();
		bool a_vacuous = g.transitions[i.index].action.isVacuous();

		if (!g_vacuous && !a_vacuous) {
			action.second = arithmetic::export_expression(g.transitions[i.index].guard, g).to_string() + " -> " +
			                arithmetic::export_composition(g.transitions[i.index].action, g).to_string();
		} else if (!g_vacuous) {
			action.second = "[" + arithmetic::export_expression(g.transitions[i.index].guard, g).to_string() + "]";
		} else {
			action.second = arithmetic::export_composition(g.transitions[i.index].action, g).to_string();
		}

		if (notations) {
			if (action.second != "") {
				action.second += "\n";
			}
			action.second += "[";
			for (int j = 0; j < (int)g.transitions[i.index].splits[petri::parallel].size(); j++) {
				if (j != 0) {
					action.second += ",";
				}
				action.second += g.transitions[i.index].splits[petri::parallel][j].to_string();
			}
			action.second += "]";
		}
		sub_result.as.push_back(action);
	}

	if (labels) {
		parse_dot::assignment label;
		label.valid = true;
		label.first = "xlabel";
		label.second = (i.type == chp::place::type ? "P" : "T") + to_string(i.index);
		sub_result.as.push_back(label);
	}

	result.attributes.push_back(sub_result);
	return result;
}

parse_dot::statement export_statement(const chp::iterator &i, const chp::graph &g, bool labels, bool notations)
{
	parse_dot::statement result;
	result.valid = true;
	result.statement_type = "node";
	result.nodes.push_back(new parse_dot::node_id(export_node_id(i)));
	result.attributes = export_attribute_list(i, g, labels, notations);
	return result;
}

parse_dot::statement export_statement(const pair<int, int> &a, const chp::graph &g, bool labels, bool notations)
{
	parse_dot::statement result;
	result.valid = true;
	result.statement_type = "edge";
	result.nodes.push_back(new parse_dot::node_id(export_node_id(g.arcs[a.first][a.second].from)));
	result.nodes.push_back(new parse_dot::node_id(export_node_id(g.arcs[a.first][a.second].to)));
	parse_dot::assignment_list attr;
	attr.valid = true;
	parse_dot::assignment label;
	label.valid = true;
	label.first = "xlabel";
	label.second = "A" + to_string(a.first) + "." + to_string(a.second);
	attr.as.push_back(label);
	if (labels)
	{
		result.attributes.valid = true;
		result.attributes.attributes.push_back(attr);
	}

	return result;
}

parse_dot::graph export_graph(const chp::graph &g, bool labels, bool notations)
{
	parse_dot::graph result;
	result.valid = true;
	result.id = g.name;
	result.type = "digraph";

	for (int i = 0; i < (int)g.places.size(); i++) {
		if (not g.places.is_valid(i)) continue;

		result.statements.push_back(export_statement(chp::iterator(chp::place::type, i), g, labels, notations));
	}

	for (int i = 0; i < (int)g.transitions.size(); i++) {
		if (not g.transitions.is_valid(i)) continue;

		result.statements.push_back(export_statement(chp::iterator(chp::transition::type, i), g, labels, notations));
	}

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < (int)g.arcs[i].size(); j++) {
			result.statements.push_back(export_statement(pair<int, int>(i, j), g, labels, notations));
		}
	}

	return result;
}

}
