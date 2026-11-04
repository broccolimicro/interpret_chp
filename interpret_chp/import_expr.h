#pragma once

#include <common/standard.h>

#include <chp/graph.h>
#include <chp/state.h>
#include <parse/tokenizer.h>

namespace chp {

struct segment {
	segment(bool cond);
	~segment();

	petri::segment nodes;
	bool loop;
	arithmetic::Expression cond;
};

segment compose(chp::graph &dst, petri::Composition composition, segment s0, segment s1, tokenizer *tokens, bool proper=true);

}
