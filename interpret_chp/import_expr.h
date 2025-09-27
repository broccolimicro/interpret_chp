#pragma once

#include <common/standard.h>

#include <chp/graph.h>
#include <chp/state.h>

#include <parse_expression/expression.h>
#include <parse_expression/assignment.h>
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
