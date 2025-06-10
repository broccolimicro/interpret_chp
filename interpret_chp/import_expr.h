#pragma once

#include <common/standard.h>

#include <chp/graph.h>
#include <chp/state.h>

#include <parse_expression/expression.h>
#include <parse_expression/assignment.h>

namespace chp {

petri::segment import_segment(chp::graph &dst, const parse_expression::expression &syntax, int default_id, tokenizer *tokens, bool auto_define);
petri::segment import_segment(chp::graph &dst, const parse_expression::assignment &syntax, int default_id, tokenizer *tokens, bool auto_define);

}
