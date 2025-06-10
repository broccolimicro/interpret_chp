#pragma once

#include <common/standard.h>

#include <parse_dot/node_id.h>
#include <parse_dot/attribute_list.h>
#include <parse_dot/statement.h>
#include <parse_dot/graph.h>

#include <chp/graph.h>
#include <chp/state.h>

namespace chp {

parse_dot::node_id export_node_id(const chp::iterator &i);
parse_dot::attribute_list export_attribute_list(const chp::iterator i, const chp::graph &g, bool labels = false, bool notations = false);
parse_dot::statement export_statement(const chp::iterator &i, const chp::graph &g, bool labels = false, bool notations = false);
parse_dot::statement export_statement(const pair<int, int> &a, const chp::graph &g, bool labels = false, bool notations = false);
parse_dot::graph export_graph(const chp::graph &g, bool labels = false, bool notations = false);

}
