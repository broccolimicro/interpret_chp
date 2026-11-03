#pragma once

#include <chp/graph.h>

#include <parse/parse.h>
#include <parse/tokenizer.h>
#include <parse/default/block_comment.h>
#include <parse/default/line_comment.h>
#include <parse_cog/composition.h>
#include <parse_cog/control.h>
#include <parse_cog/factory.h>
#include <interpret_chp/import_cog.h>

namespace test {

vector<petri::iterator> findRule(const chp::graph &g, arithmetic::Expression guard, arithmetic::Choice action);
chp::graph load_cog_string(string input);

}

