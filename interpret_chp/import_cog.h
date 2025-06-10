#pragma once

#include <common/standard.h>

#include <chp/graph.h>
#include <chp/state.h>

#include <parse_cog/composition.h>
#include <parse_cog/control.h>

#include <parse_expression/expression.h>
#include <parse_expression/assignment.h>

namespace chp {

// COG

petri::segment import_segment(chp::graph &dst, const parse_cog::composition &syntax, arithmetic::Expression &covered, bool &hasRepeat, int default_id, tokenizer *tokens, bool auto_define);
petri::segment import_segment(chp::graph &dst, const parse_cog::control &syntax, int default_id, tokenizer *tokens, bool auto_define);
void import_chp(chp::graph &dst, const parse_cog::composition &syntax, tokenizer *tokens, bool auto_define);

}
