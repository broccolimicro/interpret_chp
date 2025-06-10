#pragma once

#include <common/standard.h>

#include <chp/graph.h>
#include <chp/state.h>

#include <parse_chp/composition.h>
#include <parse_chp/control.h>

namespace chp {

// CHP

petri::segment import_segment(chp::graph &dst, const parse_chp::composition &syntax, int default_id, tokenizer *tokens, bool auto_define);
petri::segment import_segment(chp::graph &dst, const parse_chp::control &syntax, int default_id, tokenizer *tokens, bool auto_define);
void import_chp(chp::graph &dst, const parse_chp::composition &syntax, tokenizer *tokens, bool auto_define);

}
