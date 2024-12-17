/*
 * export.cpp
 *
 *  Created on: Feb 6, 2015
 *      Author: nbingham
 */

#include "export.h"

namespace chp {

pair<parse_astg::node, parse_astg::node> export_astg(parse_astg::graph &astg, const chp::graph &g, chp::iterator pos, map<chp::iterator, pair<parse_astg::node, parse_astg::node> > &nodes, ucs::variable_set &variables, string tlabel, string plabel)
{
	map<chp::iterator, pair<parse_astg::node, parse_astg::node> >::iterator loc = nodes.find(pos);
	if (loc == nodes.end()) {
		if (pos.type == chp::transition::type) {
			pair<parse_astg::node, parse_astg::node> inout;

			parse_expression::expression guard = export_expression(g.transitions[pos.index].guard, variables);
			parse_expression::composition action = export_composition(g.transitions[pos.index].action, variables);
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

parse_astg::graph export_astg(const chp::graph &g, ucs::variable_set &variables)
{
	parse_astg::graph result;

	result.name = "chp";

	// Add the variables
	for (int i = 0; i < (int)variables.nodes.size(); i++)
		result.internal.push_back(export_variable_name(i, variables));

	// Add the arcs
	map<chp::iterator, pair<parse_astg::node, parse_astg::node> > nodes;
	vector<int> forks;
	for (int i = 0; i < (int)g.transitions.size(); i++)
	{
		int curr = result.arcs.size();
		result.arcs.push_back(parse_astg::arc());
		pair<parse_astg::node, parse_astg::node> t0 = export_astg(result, g, chp::iterator(chp::transition::type, i), nodes, variables, to_string(i), to_string(i));
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

			pair<parse_astg::node, parse_astg::node> p1 = export_astg(result, g, chp::iterator(chp::place::type, n[j]), nodes, variables, to_string(n[j]), to_string(n[j]));
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
		pair<parse_astg::node, parse_astg::node> p0 = export_astg(result, g, chp::iterator(chp::place::type, forks[i]), nodes, variables, to_string(forks[i]), to_string(forks[i]));
		result.arcs[curr].nodes.push_back(p0.second);

		vector<int> n = g.next(chp::place::type, forks[i]);

		for (int j = 0; j < (int)n.size(); j++)
		{
			pair<parse_astg::node, parse_astg::node> t1 = export_astg(result, g, chp::iterator(chp::transition::type, n[j]), nodes, variables, to_string(n[j]), to_string(n[j]));
			result.arcs[curr].nodes.push_back(t1.second);
		}
	}

	// Add the initial markings
	for (int i = 0; i < (int)g.reset.size(); i++)
	{
		result.marking.push_back(pair<parse_expression::composition, vector<parse_astg::node> >());
		result.marking.back().first = export_composition(g.reset[i].encodings, variables);
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


parse_dot::node_id export_node_id(const chp::iterator &i)
{
	parse_dot::node_id result;
	result.valid = true;
	result.id = (i.type == chp::transition::type ? "T" : "P") + ::to_string(i.index);
	return result;
}

parse_dot::attribute_list export_attribute_list(const chp::iterator i, const chp::graph &g, ucs::variable_set &variables, bool labels, bool notations)
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
	} else {
		parse_dot::assignment plaintext;
		plaintext.valid = true;
		plaintext.first = "shape";
		plaintext.second = "plaintext";
		sub_result.as.push_back(plaintext);

		parse_dot::assignment action;
		action.valid = true;
		action.first = "label";
		bool g_vacuous = g.transitions[i.index].guard.is_constant();
		bool a_vacuous = g.transitions[i.index].action.is_vacuous();

		if (!g_vacuous && !a_vacuous) {
			action.second = export_expression(g.transitions[i.index].guard, variables).to_string() + " -> " +
			                export_composition(g.transitions[i.index].action, variables).to_string();
		} else if (!g_vacuous) {
			action.second = "[" + export_expression(g.transitions[i.index].guard, variables).to_string() + "]";
		} else {
			action.second = export_composition(g.transitions[i.index].action, variables).to_string();
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

parse_dot::statement export_statement(const chp::iterator &i, const chp::graph &g, ucs::variable_set &v, bool labels, bool notations)
{
	parse_dot::statement result;
	result.valid = true;
	result.statement_type = "node";
	result.nodes.push_back(new parse_dot::node_id(export_node_id(i)));
	result.attributes = export_attribute_list(i, g, v, labels, notations);
	return result;
}

parse_dot::statement export_statement(const pair<int, int> &a, const chp::graph &g, ucs::variable_set &v, bool labels, bool notations)
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
	label.first = "label";
	label.second = "A" + to_string(a.first) + "." + to_string(a.second);
	attr.as.push_back(label);
	if (labels)
	{
		result.attributes.valid = true;
		result.attributes.attributes.push_back(attr);
	}

	return result;
}

parse_dot::graph export_graph(const chp::graph &g, ucs::variable_set &v, bool labels, bool notations)
{
	parse_dot::graph result;
	result.valid = true;
	result.id = "chp";
	result.type = "digraph";

	for (int i = 0; i < (int)g.places.size(); i++)
		result.statements.push_back(export_statement(chp::iterator(chp::place::type, i), g, v, labels, notations));

	for (int i = 0; i < (int)g.transitions.size(); i++)
		result.statements.push_back(export_statement(chp::iterator(chp::transition::type, i), g, v, labels, notations));

	for (int i = 0; i < 2; i++)
		for (int j = 0; j < (int)g.arcs[i].size(); j++)
			result.statements.push_back(export_statement(pair<int, int>(i, j), g, v, labels, notations));

	return result;
}

/*parse_chp::sequence export_sequence(vector<petri::iterator> &i, const chp::graph &g, boolean::variable_set &v)
{
	parse_chp::sequence result;
	result.valid = true;

	vector<petri::iterator> choiceed;

	while (1)
	{
		if (i.size() == 1 && i[0].type == chp::transition::type && g.transitions[i[0].index].behavior == chp::transition::active)
			result.actions.push_back(new parse_boolean::internal_choice(export_internal_choice(g.transitions[i[0].index].action, v)));
		else if (i.size() == 1 && i[0].type == chp::transition::type && g.transitions[i[0].index].behavior == chp::transition::passive)
		{
			parse_chp::condition *c = new parse_chp::condition();
			c->valid = true;
			c->branches.resize(1);
			c->branches[0].first = export_disjunction(g.transitions[i[0].index].action, v);
			result.actions.push_back(c);
		}
		else if (i.size() > 1 && i[0].type == chp::place::type)
			result.actions.push_back(new parse_chp::parallel(export_parallel(i, g, v)));
		else if (i.size() > 1 && i[0].type == chp::transition::type)
			result.actions.push_back(new parse_chp::condition(export_condition(i, g, v)));

		vector<petri::iterator> n = g.next(i);
		sort(n.begin(), n.end());
		n.resize(unique(n.begin(), n.end()) - n.begin());

		vector<petri::iterator> p = g.prev(n);
		sort(p.begin(), p.end());
		p.resize(unique(p.begin(), p.end()) - p.begin());


		if (vector_intersection_size(choiceed, p) != 0 || p.size() > i.size())
			return result;
		else
		{
			choiceed.insert(choiceed.end(), i.begin(), i.end());
			i = n;
		}
	}
}

parse_chp::parallel export_parallel(vector<petri::iterator> &i, const chp::graph &g, boolean::variable_set &v)
{
	parse_chp::parallel result;
	result.valid = true;
	vector<petri::iterator> end;

	for (int j = 0; j < (int)i.size(); j++)
	{
		vector<petri::iterator> start(1, i[j]);
		result.branches = export_sequence(start, g, v);
		end.insert(end.end(), start.begin(), start.end());
	}

	i = end;

	return result;
}

parse::syntax *export_condition(vector<petri::iterator> &i, const chp::graph &g, boolean::variable_set &v)
{
	parse_chp::condition result;
	result.valid = true;
	vector<petri::iterator> end;

	for (int j = 0; j < (int)i.size(); j++)
	{
		vector<petri::iterator> start(1, i[j]);
		result.branches.push_back(pair<parse_boolean::disjunction, parse_chp::parallel>());
		result.branches.back().second.valid = true;
		parse_chp::sequence s = export_sequence(start, g, v);
		if (s.actions.size() > 0 && s.actions[0]->is_a<parse_chp::condition>() &&
			((parse_chp::condition*)s.actions[0])->branches.size() == 1 &&
			((parse_chp::condition*)s.actions[0])->branches.back().second.branches.size() == 0)
		{
			result.branches.back().first = ((parse_chp::condition*)s.actions[0])->branches.back().first;
			delete s.actions[0];
			s.actions.erase(s.actions.begin());
		}
		else
			result.branches.back().first = export_disjunction(boolean::choice(boolean::parallel()), v);

		result.branches.back().second.branches.push_back(s);

		end.insert(end.end(), start.begin(), start.end());
	}

	i = end;

	return result;
}*/

/*
parse_chp::sequence export_sequence(vector<petri::iterator> nodes, map<petri::iterator, int> counts, const chp::graph &g, const boolean::variable_set &v)
{
	// Maintain a stack to help us manage the hierarchy. The deeper
	// the stack, the more hierarchy there is. This stack stores
	// only sequences. Since we know at this point that every parallel
	// or conditional block we introduce will only have one branch, we
	// can get away with this. We will merge these sequences later to
	// add parallelism or choice.
	parse_chp::sequence head;
	head.valid = true;
	vector<parse_chp::sequence*> stack;
	stack.push_back(&head);

	int delta = 0;
	int value = counts[nodes[0]]-1;
	petri::iterator last = nodes[0];
	for (int j = 1; j < (int)nodes.size(); j++)
	{
		int c = counts[nodes[j]]-1;
		delta = c - value;
		value = c;

		// The count increased, we need to remove hierarchy
		if (delta > 0 && j > 1)
		{
			if (stack.size() == 0)
			{
				error("", "chp not properly nested", __FILE__, __LINE__);
				return parse_chp::sequence();
			}

			stack.back()->end = last.index;

			if (stack.size() > 1)
				stack[stack.size()-2]->actions.back()->end = nodes[j].index;

			stack.pop_back();
			delta--;
		}

		// The count decreased, we need to add hierarchy
		if (delta < 0)
		{
			// The last node before the count decrease was a transition, meaning
			// we need to wrap the next couple transitions in a parallel block
			if (last.type == chp::transition::type)
			{
				parse_chp::parallel *tmp = new parse_chp::parallel();
				tmp->valid = true;

				// This is very important: we need to keep track of what
				// syntaxes belong to what nodes. That way we can compare them
				// later when we go to merge them.
				tmp->start = last.index;

				parse_chp::sequence new_head;
				new_head.valid = true;
				new_head.start = nodes[j].index;
				tmp->branches.push_back(new_head);

				stack.back()->actions.push_back(tmp);
				stack.push_back(&tmp->branches.back());
			}
			// The last node before the count decrease was a place, meaning we need
			// to wrap the next couple transitions in a conditional block. Never
			// mind the guard, we will take care of that later.
			else if (last.type == chp::place::type)
			{
				parse_chp::condition *tmp = new parse_chp::condition();
				tmp->valid = true;

				// This is very important: we need to keep track of what
				// syntaxes belong to what nodes. That way we can compare them
				// later when we go to merge them.
				tmp->start = last.index;

				tmp->branches.push_back(pair<parse_boolean::guard, parse_chp::parallel>());
				tmp->branches.back().second.valid = true;
				parse_chp::sequence new_head;
				new_head.valid = true;
				new_head.start = nodes[j].index;
				tmp->branches.back().second.branches.push_back(new_head);

				stack.back()->actions.push_back(tmp);
				stack.push_back(&tmp->branches.back().second.branches.back());
			}

			delta++;
		}

		// Once we have dealt with the hierarchy issues, we still need to add assignments and disjunctions into the chp block.
		// We'll package the disjunctions into conditionals later.
		if (stack.size() == 0)
		{
			internal("", "empty stack", __FILE__, __LINE__);
			return head;
		}
		else if (nodes[j].type == chp::transition::type && g.transitions[nodes[j].index].behavior == chp::transition::active)
		{
			stack.back()->actions.push_back(new parse_boolean::assignment(export_composition(g.transitions[nodes[j].index].action, v)));
			stack.back()->actions.back()->start = nodes[j].index;
			stack.back()->actions.back()->end = nodes[j].index;
		}
		else if (nodes[j].type == chp::transition::type && g.transitions[nodes[j].index].behavior == chp::transition::passive)
		{
			stack.back()->actions.push_back(new parse_boolean::guard(export_expression_xfactor(g.transitions[nodes[j].index].action, v)));
			stack.back()->actions.back()->start = nodes[j].index;
			stack.back()->actions.back()->end = nodes[j].index;
		}

		last = nodes[j];
	}

	return head;
}


// TODO this doesn't handle the case where one sequence is a subset of another.
bool merge_sequences(parse_chp::sequence &s0, parse_chp::sequence &s1, vector<parse::syntax*> &m)
{
	int offset = 0;
	bool equal = false;
	if (s0.actions.size() >= s1.actions.size())
	{
		for (offset = 0; offset < (int)s0.actions.size() && !equal; offset++)
		{
			equal = true;
			for (int k = 0; k < (int)s1.actions.size() && equal; k++)
				if (s0.actions[(offset + k)%s0.actions.size()]->start != s1.actions[k]->start)
					equal = false;
		}
	}
	else if (s1.actions.size() > s0.actions.size())
	{
		for (offset = 0; offset < (int)s1.actions.size() && !equal; offset++)
		{
			equal = true;
			for (int k = 0; k < (int)s0.actions.size() && equal; k++)
				if (s1.actions[(offset + k)%s1.actions.size()]->start != s0.actions[k]->start)
					equal = false;
		}

		if (equal)
			swap(s0, s1);
	}

	offset--;

	for (int k = 0; k < (int)s1.actions.size() && equal; k++)
	{
		if (s0.actions[(offset+k)%s0.actions.size()]->is_a<parse_chp::parallel>() &&
			s1.actions[k]->is_a<parse_chp::parallel>())
		{
			parse_chp::parallel *tmp0 = (parse_chp::parallel *)s0.actions[(offset+k)%s0.actions.size()];
			parse_chp::parallel *tmp1 = (parse_chp::parallel *)s1.actions[k];
			tmp0->branches.insert(tmp0->branches.end(), tmp1->branches.begin(), tmp1->branches.end());
			m.push_back(tmp0);
		}
		else if (s0.actions[(offset+k)%s0.actions.size()]->is_a<parse_chp::condition>() &&
				s1.actions[k]->is_a<parse_chp::condition>())
		{
			parse_chp::condition *tmp0 = (parse_chp::condition *)s0.actions[(offset+k)%s0.actions.size()];
			parse_chp::condition *tmp1 = (parse_chp::condition *)s1.actions[k];
			tmp0->branches.insert(tmp0->branches.end(), tmp1->branches.begin(), tmp1->branches.end());
			m.push_back(tmp0);
		}
	}

	return equal;
}

// TODO this doesn't know how to handle non-deterministic choice yet.
// The result is that all non-deterministic conditionals are converted to deterministic ones.
// TODO This function does not always throw an error when an HSE is not properly nested.
// It will just return a bunch of independent processes that don't correctly implement the HSE.
parse_chp::parallel export_parallel(const chp::graph &g, const boolean::variable_set &v)
{
	// First step is to identify all of the cycles in the graph.
	vector<vector<petri::iterator> > cycles = g.cycles();

	for (int i = 0; i < (int)cycles.size(); i++)
	{
		printf("{");
		for (int j = 0; j < (int)cycles[i].size(); j++)
		{
			if (j != 0)
				printf(", ");
			printf("%c%d", cycles[i][j].type == chp::place::type ? 'P' : 'T', cycles[i][j].index);
		}
		printf("}\n");
	}

	// We then need to figure out where add hierarchy. We can do this by looking at the
	// number of cycles that share a given node. If that number decreases, then we need
	// to increase the hierarchy level. If that number increases, then we need to decrease
	// the hierarchy level. Knowing this, we can just iterate over all the nodes in each
	// cycle and add either a condition or a parallel depending upon the type of the last
	// node we passed.
	map<petri::iterator, int> counts;
	for (int i = 0; i < (int)cycles.size(); i++)
		count_elements(cycles[i], counts);

	parse_chp::parallel wrapper;
	wrapper.valid = true;
	for (int i = 0; i < (int)cycles.size(); i++)
	{
		// Put all of the generated sequences into one parallel block. Remember that
		// these sequences still need to be wrapped in a loop, but we'll do that in a later step.
		wrapper.branches.push_back(export_sequence(cycles[i], counts, g, v));
	}

	//printf("%s\n", wrapper.to_string().c_str());

	// Our next step is to merge corresponding sequences in this wrapper.
	// If the HSE is properly nested then we can merge two sequences if
	// one is a subset or equals another. When we run up against a conditional
	// or parallel block that exists in both, we merge them, and we can just
	// ignore everything else. This process needs to happen recursively.
	// (though the data recursion is not implemented as a function recursion)
	vector<parse::syntax*> m;
	m.push_back(&wrapper);

	while (m.size() > 0)
	{
		sort(m.begin(), m.end());
		m.resize(unique(m.begin(), m.end()) - m.begin());
		parse::syntax *syn = m.back();
		m.pop_back();

		if (syn->is_a<parse_chp::parallel>())
		{
			parse_chp::parallel *par = (parse_chp::parallel *)syn;

			for (int i = 0; i < (int)par->branches.size(); i++)
				for (int j = (int)par->branches.size()-1; j >= i+1; j--)
					if (merge_sequences(par->branches[i], par->branches[j], m))
						par->branches.erase(par->branches.begin() + j);
		}
		else if (syn->is_a<parse_chp::condition>())
		{
			parse_chp::condition *par = (parse_chp::condition *)syn;

			for (int i = 0; i < (int)par->branches.size(); i++)
				for (int j = (int)par->branches.size()-1; j >= i+1; j--)
					if (merge_sequences(par->branches[i].second.branches[0], par->branches[j].second.branches[0], m))
						par->branches.erase(par->branches.begin() + j);
		}
	}

	//printf("%s\n", wrapper.to_string().c_str());

	// Next, we need to fix the syntax for all of the conditionals by finding their appropriate disjunctions
	// and moving them to the right slots. We also wrap guards in their own conditional here. This process
	// requires recursion (again, not functionally implemented).
	m.push_back(&wrapper);
	while(m.size() > 0)
	{
		parse::syntax *syn = m.back();
		m.pop_back();

		// We've run up against a conditional
		if (syn->is_a<parse_chp::condition>())
		{
			parse_chp::condition *c = (parse_chp::condition*)syn;
			for (int i = 0; i < (int)c->branches.size(); i++)
			{
				// Check to see if the first item in the sequence of this branch is a disjunction
				if (c->branches[i].second.branches.size() == 1 && c->branches[i].second.branches[0].actions.size() > 0 && c->branches[i].second.branches[0].actions[0]->is_a<parse_boolean::guard>())
				{
					// if it is, we can move it to the guard
					c->branches[i].first = *(parse_boolean::guard*)c->branches[i].second.branches[0].actions[0];
					delete c->branches[i].second.branches[0].actions[0];
					c->branches[i].second.branches[0].actions.erase(c->branches[i].second.branches[0].actions.begin());
				}
				else
					// if it isn't then we need to write our own guard.
					c->branches[i].first = export_expression(boolean::parallel(), v);

				// recurse
				m.push_back(&c->branches[i].second);
			}
		}
		// parallel block, we just recurse here
		else if (syn->is_a<parse_chp::parallel>())
		{
			parse_chp::parallel *p = (parse_chp::parallel*)syn;
			for (int i = 0; i < (int)p->branches.size(); i++)
				m.push_back(&p->branches[i]);
		}
		// A sequence, find all of the disjunctions that don't belong to conditionals
		// and give them their own conditional. Then, recurse.
		else if (syn->is_a<parse_chp::sequence>())
		{
			parse_chp::sequence *s = (parse_chp::sequence*)syn;
			for (int i = 0; i < (int)s->actions.size(); i++)
			{
				if (s->actions[i]->is_a<parse_boolean::guard>())
				{
					parse_chp::condition *c = new parse_chp::condition();
					c->valid = true;
					c->deterministic = true;
					c->branches.resize(1);
					c->branches.back().first = *((parse_boolean::guard*)s->actions[i]);
					c->start = c->branches.back().first.start;
					delete s->actions[i];
					s->actions[i] = c;
				}
				else
					m.push_back(s->actions[i]);
			}
		}
	}

	//printf("%s\n", wrapper.to_string().c_str());

	// Finally, just wrap all of the sequences in infinite loops and call it a day.
	for (int i = 0; i < (int)wrapper.branches.size(); i++)
	{
		parse_chp::loop *l = new parse_chp::loop();
		l->valid = true;
		l->branches.resize(1);
		l->branches[0].second.branches.resize(1);
		l->branches[0].second.branches[0] = wrapper.branches[i];
		wrapper.branches[i].clear();
		wrapper.branches[i].actions.push_back(l);
	}

	//printf("%s\n", wrapper.to_string().c_str());

	return wrapper;
}*/

string export_transition(const chp::graph &g, const ucs::variable_set &v, petri::iterator i, bool is_guard, bool here) {
	string result;

	bool render_guard = not g.transitions[i.index].guard.is_valid();
	bool render_action = not g.transitions[i.index].action.is_vacuous();

	if (is_guard) {
		if (render_guard) {
			result += export_expression(g.transitions[i.index].guard, v).to_string() + "->";
		} else {
			result += "vdd->";
		}
	} else {
		if (render_guard) {
			result += "[" + export_expression(g.transitions[i.index].guard, v).to_string() + "]";
		}

		if (render_guard and render_action) {
			result += ";";
		}
	}

	if (here) {
		result += "  <here>  ";
	}

	if (render_action) {
		result += export_composition(g.transitions[i.index].action, v).to_string();
	}

	if ((is_guard or not render_guard) and not render_action) {
		result += "skip";
	}
	return result;
}

string export_node(petri::iterator i, const chp::graph &g, const ucs::variable_set &v)
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

			result += export_transition(g, v, pp[j], false, false);
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
		result += ";" + export_transition(g, v, i, false, true) + ";";
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


			result += export_transition(g, v, nn[j], nn.size() > 1, false);
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
