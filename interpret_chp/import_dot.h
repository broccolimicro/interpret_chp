#pragma once

#include <common/standard.h>

#include <chp/graph.h>
#include <chp/state.h>

#include <parse_dot/node_id.h>
#include <parse_dot/assignment.h>
#include <parse_dot/assignment_list.h>
#include <parse_dot/statement.h>
#include <parse_dot/graph.h>

#include <parse_expression/expression.h>
#include <parse_expression/assignment.h>

namespace chp {

// DOT

petri::iterator import_chp(const parse_dot::node_id &syntax, map<string, petri::iterator> &nodes, chp::graph &g, tokenizer *token, bool define, bool squash_errors);
map<string, string> import_chp(const parse_dot::attribute_list &syntax, tokenizer *tokens);
void import_chp(const parse_dot::statement &syntax, chp::graph &g, map<string, map<string, string> > &globals, map<string, petri::iterator> &nodes, tokenizer *tokens, bool auto_define);
void import_chp(const parse_dot::graph &syntax, chp::graph &g, map<string, map<string, string> > &globals, map<string, petri::iterator> &nodes, tokenizer *tokens, bool auto_define);
chp::graph import_chp(const parse_dot::graph &syntax, tokenizer *tokens, bool auto_define);

}
