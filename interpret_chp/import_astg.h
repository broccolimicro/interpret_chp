#pragma once

#include <common/standard.h>

#include <chp/graph.h>
#include <chp/state.h>

#include <parse_astg/node.h>
#include <parse_astg/arc.h>
#include <parse_astg/graph.h>

#include <parse_expression/expression.h>
#include <parse_expression/assignment.h>

namespace chp {

// ASTG

chp::iterator import_chp(const parse_astg::node &syntax, chp::graph &g, tokenizer *token);
void import_chp(const parse_astg::arc &syntax, chp::graph &g, tokenizer *tokens);
chp::graph import_chp(const parse_astg::graph &syntax, tokenizer *tokens);

}
