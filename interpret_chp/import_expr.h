#pragma once

#include <common/standard.h>

#include <chp/graph.h>
#include <chp/state.h>

#include <parse_chp/expression.h>
#include <interpret_arithmetic/import.h>

namespace chp {

struct segment {
	segment(bool cond);
	~segment();

	petri::segment nodes;
	bool loop;
	arithmetic::Expression cond;
};

segment compose(chp::graph &dst, int composition, segment s0, segment s1);

struct ExpressionInterpreter : arithmetic::ExpressionInterpreter<parse_chp::expr_group> {
	ExpressionInterpreter(tokenizer *tokens=nullptr, bool auto_define=true);
	ExpressionInterpreter();

	arithmetic::Expression import_unary(parse_expression::operation op, arithmetic::Expression expr);
	arithmetic::Expression import_binary(parse_expression::operation op, arithmetic::Expression left, arithmetic::Expression right);
	arithmetic::Expression import_group(parse_expression::operation op, vector<arithmetic::Expression> args);
	arithmetic::Expression import_modifier(parse_expression::operation op, const vector<argument> &arguments, ucs::Netlist nets, int default_id);
};

arithmetic::Expression import_expression(const parse_chp::expression &syntax, ucs::Netlist nets, int default_id=0, tokenizer *tokens=nullptr, bool auto_define=true);
arithmetic::State import_state(const parse_chp::simple_composition &syntax, ucs::Netlist nets, int default_id=0, tokenizer *tokens=nullptr, bool auto_define=true);

template <int group, typename number_t, typename instance_t>
segment import_segment(chp::graph &dst, const parse_expression::expression_t<group, number_t, instance_t> &syntax, string func, int default_id, tokenizer *tokens, bool auto_define);
template <int group, typename number_t, typename instance_t>
segment import_segment(chp::graph &dst, const parse_expression::assignment_t<group, number_t, instance_t> &syntax, int default_id, tokenizer *tokens, bool auto_define);

template <int group, typename number_t, typename instance_t>
segment import_segment(chp::graph &dst, const parse_expression::expression_t<group, number_t, instance_t> &syntax, string func, int default_id, tokenizer *tokens, bool auto_define) {
	segment result(false);
	result.cond = arithmetic::import_expression(syntax, dst, default_id, tokens, auto_define);
	if (func == "await") {
		result.cond = arithmetic::isValid(result.cond);
	} else if (func == "if") {
		result.cond = arithmetic::isTrue(result.cond);
	}
	petri::iterator t = dst.create(chp::transition(result.cond));
	result.nodes = petri::segment({{t}}, {{t}});
	return result;
}

template <int group, typename number_t, typename instance_t>
segment import_segment(chp::graph &dst, const parse_expression::assignment_t<group, number_t, instance_t> &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	static const auto Vdd = arithmetic::Operand::vdd();

	segment result(true);
	petri::iterator t = dst.create(chp::transition(Vdd, {{arithmetic::import_action(syntax, dst, default_id, tokens, auto_define)}}));
	result.nodes = petri::segment({{t}}, {{t}});
	return result;
}


}
