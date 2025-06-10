#pragma once

#include <chp/graph.h>

namespace test {

vector<petri::iterator> findRule(const chp::graph &g, arithmetic::Expression guard, arithmetic::Choice action);

}

