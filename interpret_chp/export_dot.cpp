#include "export_dot.h"

namespace chp {

parse_dot::node_id export_node_id(const chp::iterator &i)
{
	parse_dot::node_id result;
	result.valid = true;
	result.id = (i.type == chp::transition::type ? "T" : "P") + ::to_string(i.index);
	return result;
}

parse_dot::attribute_list export_attribute_list(const chp::iterator i, const chp::graph &g, const petri::CompositionAnalysis &comp, bool labels) {
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
			if (comp.empty()) {
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

		if (not comp.empty()) {
			if (encoding.second != "") {
				encoding.second += "\n";
			}
			encoding.second += "[";
			for (int j = 0; j < (int)comp.places[i.index].splits[petri::PARALLEL].size(); j++) {
				if (j != 0) {
					encoding.second += ",";
				}
				encoding.second += comp.places[i.index].splits[petri::PARALLEL][j].to_string();
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
			action.second = g.transitions[i.index].guard.to_string(false, g) + " -> " +
			                g.transitions[i.index].action.to_string(false, g);
		} else if (!g_vacuous) {
			action.second = g.transitions[i.index].guard.to_string(false, g) + " -> skip";
		} else {
			action.second = g.transitions[i.index].action.to_string(false, g);
		}

		if (not comp.empty()) {
			if (action.second != "") {
				action.second += "\n";
			}
			action.second += "[";
			for (int j = 0; j < (int)comp.transitions[i.index].splits[petri::PARALLEL].size(); j++) {
				if (j != 0) {
					action.second += ",";
				}
				action.second += comp.transitions[i.index].splits[petri::PARALLEL][j].to_string();
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

parse_dot::statement export_statement(const chp::iterator &i, const chp::graph &g, const petri::CompositionAnalysis &comp, bool labels) {
	parse_dot::statement result;
	result.valid = true;
	result.statement_type = "node";
	result.nodes.push_back(new parse_dot::node_id(export_node_id(i)));
	result.attributes = export_attribute_list(i, g, comp, labels);
	return result;
}

parse_dot::statement export_statement(const pair<int, int> &a, const chp::graph &g, bool labels) {
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
	if (labels) {
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

	petri::CompositionAnalysis comp;
	if (notations) {
		comp.build(g.adjacency());
	}

	for (int i = 0; i < (int)g.places.size(); i++) {
		if (not g.places.is_valid(i)) continue;

		result.statements.push_back(export_statement(chp::iterator(chp::place::type, i), g, comp, labels));
	}

	for (int i = 0; i < (int)g.transitions.size(); i++) {
		if (not g.transitions.is_valid(i)) continue;

		result.statements.push_back(export_statement(chp::iterator(chp::transition::type, i), g, comp, labels));
	}

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < (int)g.arcs[i].size(); j++) {
			result.statements.push_back(export_statement(pair<int, int>(i, j), g, labels));
		}
	}

	return result;
}

parse_dot::graph export_analysis(const chp::graph &g) {
	parse_dot::graph result;
	result.valid = true;
	result.id = g.name + "_analysis";
	result.type = "digraph";

	// Create a node for each variable
	for (const auto &[var_idx, chain] : g.useDefChains) {
		string var_name = chain.name;
		string var_node_id = "var_" + to_string(var_idx);

		parse_dot::statement var_stmt;
		var_stmt.valid = true;
		var_stmt.statement_type = "node";
		var_stmt.nodes.push_back(new parse_dot::node_id(var_node_id));

		parse_dot::attribute_list var_attrs;
		var_attrs.valid = true;
		parse_dot::assignment_list var_sub_attrs;
		var_sub_attrs.valid = true;

		parse_dot::assignment var_shape;
		var_shape.valid = true;
		var_shape.first = "shape";
		var_shape.second = "box";
		var_sub_attrs.as.push_back(var_shape);

		parse_dot::assignment var_label;
		var_label.valid = true;
		var_label.first = "label";
		var_label.second = var_name;
		var_sub_attrs.as.push_back(var_label);

		var_attrs.attributes.push_back(var_sub_attrs);
		var_stmt.attributes = var_attrs;
		result.statements.push_back(var_stmt);
	}

	// Create use-def edges: from used variable to defined variable, labeled with transition index
	// For each transition, find which variables it uses and defines
	for (TransitionIdx trans_idx = 0; trans_idx < g.transitions.size(); trans_idx++) {
		if (!g.transitions.is_valid(trans_idx)) continue;

		const chp::transition &tran = g.transitions[trans_idx];

		// Check for channel sends - treat output channel as definer, expression vars as used
		vector<VarIdx> output_channels;
		vector<VarIdx> input_channels;
		for (const auto &term : tran.action.terms) {
			for (const auto &action : term.actions) {
				vector<VarIdx> sends = findOutputChannelsInExpression(action.rvalue);
				vector<VarIdx> recvs = findInputChannelsInExpression(action.rvalue);
				output_channels.insert(output_channels.end(), sends.begin(), sends.end());
				input_channels.insert(input_channels.end(), recvs.begin(), recvs.end());
			}
		}

		set<VarIdx> used_vars;
		set<VarIdx> defined_vars;
		bool is_channel_send = !output_channels.empty();
		bool is_channel_recv = !input_channels.empty();

		if (is_channel_send) {
			// Channel send semantics: output channel is definer, expression vars are used
			for (VarIdx output_channel : output_channels) {
				defined_vars.insert(output_channel);
			}

			// Find all variables used in the sent expressions
			for (const auto &term : tran.action.terms) {
				for (const auto &action : term.actions) {
					vector<VarIdx> expr_vars = getVarsFromExpression(action.rvalue);
					for (VarIdx var : expr_vars) {
						// Exclude the output channel itself from used vars
						if (std::find(output_channels.begin(), output_channels.end(), var) == output_channels.end()) {
							used_vars.insert(var);
						}
					}
				}
			}
		} else {
			// Regular assignment semantics
			for (const auto &[var_idx, chain] : g.useDefChains) {
				for (TransitionIdx use_idx : chain.uses) {
					if (use_idx == trans_idx) {
						used_vars.insert(var_idx);
					}
				}
			}

			for (const auto &[var_idx, chain] : g.useDefChains) {
				for (TransitionIdx def_idx : chain.defs) {
					if (def_idx == trans_idx) {
						defined_vars.insert(var_idx);
					}
				}
			}
		}

		// Create edges from each used var to each defined var
		for (VarIdx used_var : used_vars) {
			for (VarIdx defined_var : defined_vars) {
				string from_node_id = "var_" + to_string(used_var);
				string to_node_id = "var_" + to_string(defined_var);

				parse_dot::statement edge_stmt;
				edge_stmt.valid = true;
				edge_stmt.statement_type = "edge";
				edge_stmt.nodes.push_back(new parse_dot::node_id(from_node_id));
				edge_stmt.nodes.push_back(new parse_dot::node_id(to_node_id));

				parse_dot::attribute_list edge_attrs;
				edge_attrs.valid = true;
				parse_dot::assignment_list edge_sub_attrs;
				edge_sub_attrs.valid = true;

				parse_dot::assignment edge_label;
				edge_label.valid = true;
				edge_label.first = "label";
				edge_label.second = to_string(trans_idx);
				edge_sub_attrs.as.push_back(edge_label);

				// Color edges based on channel operation
				if (is_channel_send) {
					parse_dot::assignment edge_color;
					edge_color.valid = true;
					edge_color.first = "color";
					edge_color.second = "red";
					edge_sub_attrs.as.push_back(edge_color);
				} else if (is_channel_recv) {
					parse_dot::assignment edge_color;
					edge_color.valid = true;
					edge_color.first = "color";
					edge_color.second = "blue";
					edge_sub_attrs.as.push_back(edge_color);
				}

				edge_attrs.attributes.push_back(edge_sub_attrs);
				edge_stmt.attributes = edge_attrs;
				result.statements.push_back(edge_stmt);
			}
		}
	}

	return result;
}

}
