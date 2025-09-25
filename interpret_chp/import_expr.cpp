#include "import_expr.h"
#include <interpret_arithmetic/import.h>
#include <interpret_arithmetic/export.h>

namespace chp {

segment::segment(bool cond) {
	this->loop = false;
	this->cond = cond ? arithmetic::Expression::vdd() : arithmetic::Expression::gnd();
}

segment::~segment() {
}

segment compose(chp::graph &dst, int composition, segment s0, segment s1) {
	if (composition == petri::choice) {
		s0.cond = s0.cond | s1.cond;
	} else if (composition == petri::parallel) {
		s0.cond = s0.cond & s1.cond;
	} else if (composition == petri::sequence) {
		if (s0.nodes.source.empty()) {
			s0.cond = s1.cond;
			s0.loop = s1.loop;
		}
	}
	s0.nodes = dst.compose(composition, s0.nodes, s1.nodes);
	return s0;
}

segment import_segment(chp::graph &dst, const parse_expression::expression &syntax, string func, int default_id, tokenizer *tokens, bool auto_define) {
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

segment import_segment(chp::graph &dst, const parse_expression::assignment &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	static const auto Vdd = arithmetic::Operand::vdd();

	segment result(true);
	petri::iterator t = dst.create(chp::transition(Vdd, {{arithmetic::import_action(syntax, dst, default_id, tokens, auto_define)}}));
	result.nodes = petri::segment({{t}}, {{t}});
	return result;
}

}
