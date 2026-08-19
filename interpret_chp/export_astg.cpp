#include "export_astg.h"
#include <interpret_arithmetic/export.h>
#include <parse_astg/expression.h>

namespace parse_astg {

ExpressionExporter::ExpressionExporter(ucs::ConstNetlist nets) : nets(nets) {
}

ExpressionExporter::~ExpressionExporter() {
}

parse_expression::operation ExpressionExporter::export_operator(int func) const {
	using OpType = arithmetic::Operation::OpType;
	using operation = parse_expression::operation;

	switch (func) {
	// VALIDITY - converted to CALL
	case OpType::WIRE_NOT: return operation("~", "", "", "");
	case OpType::WIRE_OR:  return operation("", "", "|", "");
	case OpType::WIRE_AND: return operation("", "", "&", "");
	case OpType::WIRE_XOR: return operation("", "", "^", "");
	// TRUTHINESS - converted to CALL
	case OpType::BOOLEAN_NOT: return operation("!", "", "", "");
	case OpType::BOOLEAN_OR: return operation("", "", "||", "");
	case OpType::BOOLEAN_AND: return operation("", "", "&&", "");
	case OpType::BOOLEAN_XOR: return operation("", "", "^^", "");
	case OpType::EQUAL: return operation("", "", "==", "");
	case OpType::NOT_EQUAL: return operation("", "", "!=", "");
	case OpType::LESS: return operation("", "", "<", "");
	case OpType::GREATER: return operation("", "", ">", "");
	case OpType::LESS_EQUAL: return operation("", "", "<=", "");
	case OpType::GREATER_EQUAL: return operation("", "", ">=", "");
	// NEGATIVE - converted to LESS
	case OpType::TERNARY: return operation("", "?", ":", "");
	case OpType::IDENTITY: return operation("+", "", "", "");
	case OpType::NEGATION: return operation("-", "", "", "");
	// INVERSE - converted to INTDIV
	// TODO(edward.bingham) we need type information here to determine if we are using arithmetic or logical shift
	case OpType::SHIFT_LEFT: return operation("", "", "<<", "");
	case OpType::SHIFT_RIGHT: return operation("", "", ">>", "");
	case OpType::ADD: return operation("", "", "+", "");
	case OpType::SUBTRACT: return operation("", "", "-", "");
	case OpType::MULTIPLY: return operation("", "", "*", "");
	case OpType::INTDIV: return operation("", "", "/", "");
	case OpType::INTMOD: return operation("", "", "%", "");
	case OpType::CALL: return operation("", "(", ",", ")");
	// MEMBER_CALL - converted to MEMBER and CALL
	//case OpType::CAST: return operation("", "(", "", ")");
	case OpType::ARRAY: return operation("[", "", ",", "]");
	case OpType::INDEX: return operation("", "[", ":", "]");
	//case OpType::STRUCT: return operation("'{", "", "", "}");
	case OpType::MEMBER: return operation("", ".", "", "");
	}
	return operation();
}

const parse_expression::precedence_set &ExpressionExporter::precedence() const {
	return expression_config::cfg->order;
}

parse_expression::expression::argument ExpressionExporter::export_constant(arithmetic::Value value) const {
	if (value.type == arithmetic::Value::LABEL) {
		label result;
		result.value = arithmetic::export_value(value);
		return {2, std::shared_ptr<parse::syntax>(result.clone())};
	}
	constant result;
	result.value = arithmetic::export_value(value);
	return {0, std::shared_ptr<parse::syntax>(result.clone())};
}

parse_expression::expression::argument ExpressionExporter::export_literal(size_t index) const {
	literal result;
	result.name = nets.netAt(index);
	return {1, std::shared_ptr<parse::syntax>(result.clone())};
}

parse_expression::expression export_expression(const arithmetic::Expression &expr, ucs::ConstNetlist nets) {
	return ExpressionExporter(nets).export_expression(expr);
}

parse_expression::assignment export_assignment(const arithmetic::Action &expr, ucs::ConstNetlist nets) {
	parse_expression::assignment result;
	result.valid = true;

	if (not expr.lvalue.isUndef()) {
		result.left.push_back(export_expression(expr.lvalue, nets));
	}

	// TODO(edward.bingham) we need type information about the lvalue here
	arithmetic::Operand top = expr.rvalue.top;
	if (top.isConst() and top.cnst.isNeutral()) {
		result.operation = "-";
	} else if (top.isConst() and top.cnst.isUnstable()) {
		result.operation = "~";
	} else if (top.isConst() and top.cnst.type == arithmetic::Value::WIRE and top.cnst.isValid()) {
		result.operation = "+";
	} else {
		result.right = export_expression(expr.rvalue, nets);
		result.operation = "=";
	}

	return result;
}

CompositionExporter::CompositionExporter(ucs::ConstNetlist nets) : nets(nets) {
}

CompositionExporter::~CompositionExporter() {
}

parse_expression::operation CompositionExporter::export_operator(int func) const {
	using OpType = arithmetic::Operation::OpType;
	using operation = parse_expression::operation;

	switch (func) {
	// VALIDITY - converted to CALL
	case OpType::WIRE_OR:  return operation("", "", ":", "");
	case OpType::WIRE_AND: return operation("", "", ",", "");
	}
	return operation();
}

const parse_expression::precedence_set &CompositionExporter::precedence() const {
	return composition_config::cfg->order;
}

parse_expression::expression::argument CompositionExporter::export_action(const arithmetic::Action &expr) const {
	return {1, std::shared_ptr<parse::syntax>(export_assignment(expr, nets).clone())};
}

parse_expression::expression export_composition(const arithmetic::Parallel &expr, ucs::ConstNetlist nets) {
	return CompositionExporter(nets).export_expression(expr);
}

parse_expression::expression export_composition(const arithmetic::Choice &expr, ucs::ConstNetlist nets) {
	return CompositionExporter(nets).export_expression(expr);
}


pair<parse_astg::node, parse_astg::node> export_astg(parse_astg::graph &astg, const chp::graph &g, chp::iterator pos, map<chp::iterator, pair<parse_astg::node, parse_astg::node> > &nodes, string tlabel, string plabel)
{
	map<chp::iterator, pair<parse_astg::node, parse_astg::node> >::iterator loc = nodes.find(pos);
	if (loc == nodes.end()) {
		if (pos.type == chp::transition::type) {
			pair<parse_astg::node, parse_astg::node> inout;

			parse_astg::expression guard = export_expression(g.transitions[pos.index].guard, g);
			parse_astg::composition action = export_composition(g.transitions[pos.index].action, g);
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

parse_astg::graph export_astg(const chp::graph &g) {
	parse_astg::graph result;

	result.name = "chp";

	// Add the variables
	for (int i = 0; i < (int)g.vars.size(); i++)
		result.internal.push_back(export_expression(arithmetic::Expression::varOf(i), g));

	// Add the arcs
	map<chp::iterator, pair<parse_astg::node, parse_astg::node> > nodes;
	vector<int> forks;
	for (int i = 0; i < (int)g.transitions.size(); i++) {
		if (not g.transitions.is_valid(i)) continue;

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
		result.marking.push_back(pair<parse_astg::composition, vector<parse_astg::node> >());
		result.marking.back().first = export_composition(arithmetic::Parallel(g.reset[i].encodings), g);
		for (int j = 0; j < (int)g.reset[i].tokens.size(); j++)
			result.marking.back().second.push_back(parse_astg::node("p" + to_string(g.reset[i].tokens[j].index)));
	}

	// Add the arbiters
	for (int i = 0; i < (int)g.places.size(); i++) {
		if (not g.places.is_valid(i)) continue;

		if (g.places[i].arbiter) {
			result.arbiter.push_back(parse_astg::node("p" + to_string(i)));
		}
	}

	return result;
}

}
