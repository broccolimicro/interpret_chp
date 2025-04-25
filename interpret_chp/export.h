#pragma once

#include <common/standard.h>

#include <parse_expression/expression.h>
#include <parse_expression/assignment.h>

#include <parse_dot/node_id.h>
#include <parse_dot/attribute_list.h>
#include <parse_dot/statement.h>
#include <parse_dot/graph.h>

#include <parse_astg/graph.h>

#include <chp/graph.h>
#include <chp/state.h>

#include <interpret_arithmetic/export.h>

namespace chp {

// ASTG

pair<parse_astg::node, parse_astg::node> export_astg(parse_astg::graph &g, parse_expression::composition c, string label);
parse_astg::graph export_astg(const chp::graph &g);

// DOT

parse_dot::node_id export_node_id(const chp::iterator &i);
parse_dot::attribute_list export_attribute_list(const chp::iterator i, const chp::graph &g, bool labels = false, bool notations = false);
parse_dot::statement export_statement(const chp::iterator &i, const chp::graph &g, bool labels = false, bool notations = false);
parse_dot::statement export_statement(const pair<int, int> &a, const chp::graph &g, bool labels = false, bool notations = false);
parse_dot::graph export_graph(const chp::graph &g, bool labels = false, bool notations = false);

// CHP

/*parse_chp::sequence export_sequence(vector<chp::iterator> &i, const chp::graph &g);
parse_chp::parallel export_parallel(vector<chp::iterator> &i, const chp::graph &g);
parse::syntax *export_condition(vector<chp::iterator> &i, const chp::graph &g);*/

/*parse_chp::parallel export_parallel(const chp::graph &g);*/

string export_node(petri::iterator i, const chp::graph &g);

}
