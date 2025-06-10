#pragma once

#include <common/standard.h>

#include <chp/graph.h>
#include <chp/state.h>

namespace chp {

string export_transition(const chp::graph &g, petri::iterator i, bool is_guard, bool here);
string export_node(petri::iterator i, const chp::graph &g);

}
