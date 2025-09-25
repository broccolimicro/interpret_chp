#pragma once

#include <common/standard.h>

#include <chp/graph.h>
#include <chp/state.h>

#include <parse_expression/expression.h>
#include <parse_expression/assignment.h>

namespace chp {

struct segment {
	segment(bool cond);
	~segment();

	petri::segment nodes;
	bool loop;
	arithmetic::Expression cond;
};

segment compose(chp::graph &dst, int composition, segment s0, segment s1);

segment import_segment(chp::graph &dst, const parse_expression::expression &syntax, string func, int default_id, tokenizer *tokens, bool auto_define);
segment import_segment(chp::graph &dst, const parse_expression::assignment &syntax, int default_id, tokenizer *tokens, bool auto_define);

}
