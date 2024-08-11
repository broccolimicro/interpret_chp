#pragma once

#include <common/standard.h>

#include <ucs/variable.h>
#include <chp/graph.h>
#include <chp/state.h>

#include <parse_astg/node.h>
#include <parse_astg/arc.h>
#include <parse_astg/graph.h>

#include <parse_dot/node_id.h>
#include <parse_dot/assignment.h>
#include <parse_dot/assignment_list.h>
#include <parse_dot/statement.h>
#include <parse_dot/graph.h>

#include <parse_chp/composition.h>
#include <parse_chp/control.h>

#include <parse_expression/expression.h>
#include <parse_expression/assignment.h>

#include <interpret_ucs/import.h>

namespace chp {

// ASTG

chp::iterator import_chp(const parse_astg::node &syntax, ucs::variable_set &variables, chp::graph &g, tokenizer *token);
void import_chp(const parse_astg::arc &syntax, ucs::variable_set &variables, chp::graph &g, tokenizer *tokens);
chp::graph import_chp(const parse_astg::graph &syntax, ucs::variable_set &variables, tokenizer *tokens);

// DOT

petri::iterator import_chp(const parse_dot::node_id &syntax, map<string, petri::iterator> &nodes, ucs::variable_set &variables, chp::graph &g, tokenizer *token, bool define, bool squash_errors);
map<string, string> import_chp(const parse_dot::attribute_list &syntax, tokenizer *tokens);
void import_chp(const parse_dot::statement &syntax, chp::graph &g, ucs::variable_set &variables, map<string, map<string, string> > &globals, map<string, petri::iterator> &nodes, tokenizer *tokens, bool auto_define);
void import_chp(const parse_dot::graph &syntax, chp::graph &g, ucs::variable_set &variables, map<string, map<string, string> > &globals, map<string, petri::iterator> &nodes, tokenizer *tokens, bool auto_define);
chp::graph import_chp(const parse_dot::graph &syntax, ucs::variable_set &variables, tokenizer *tokens, bool auto_define);

// CHP

chp::graph import_chp(const parse_expression::expression &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define);
chp::graph import_chp(const parse_expression::assignment &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define);
chp::graph import_chp(const parse_chp::composition &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define);
chp::graph import_chp(const parse_chp::control &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define);

}
