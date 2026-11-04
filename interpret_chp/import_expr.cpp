#include "import_expr.h"

namespace chp {

segment::segment(bool cond) {
	this->loop = false;
	this->cond = cond ? arithmetic::Expression::vdd() : arithmetic::Expression::gnd();
}

segment::~segment() {
}

segment compose(chp::graph &dst, petri::Composition composition, segment s0, segment s1, tokenizer *tokens, bool proper) {
	if (composition == petri::SEQUENCE and not s0.nodes.source.empty()) {
		if (s0.nodes.sink.empty()) {
			if (tokens != nullptr) {
				tokens->error("sequence composition with non-terminating process creates dead code", __FILE__, __LINE__);
			} else {
				error("", "sequence composition with non-terminating process creates dead code", __FILE__, __LINE__);
			}
		}
		if (s1.nodes.source.empty()) {
			if (tokens != nullptr) {
				tokens->error("process has no entry, sequence composition creates dead code", __FILE__, __LINE__);
			} else {
				error("", "process has no entry, sequence composition creates dead code", __FILE__, __LINE__);
			}
		}
	}

	if (composition == petri::CHOICE) {
		s0.cond = s0.cond | s1.cond;
	} else if (composition == petri::PARALLEL) {
		s0.cond = s0.cond & s1.cond;
	} else if (composition == petri::SEQUENCE) {
		if (s0.nodes.source.empty()) {
			s0.cond = s1.cond;
			s0.loop = s1.loop;
		}
	}
	s0.nodes = dst.compose(composition, s0.nodes, s1.nodes, proper);
	return s0;
}

}
