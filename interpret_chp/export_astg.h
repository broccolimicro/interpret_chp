#pragma once

#include <common/standard.h>

#include <parse_astg/graph.h>

#include <chp/graph.h>
#include <chp/state.h>

namespace chp {

// ASTG

pair<parse_astg::node, parse_astg::node> export_astg(parse_astg::graph &g, parse_expression::composition c, string label);
parse_astg::graph export_astg(const chp::graph &g);

}
