#include <common/standard.h>
#include <gtest/gtest.h>

#include <chp/graph.h>
#include <parse/tokenizer.h>
#include <parse/default/block_comment.h>
#include <parse/default/line_comment.h>
#include <parse_chp/composition.h>
#include <interpret_chp/import_chp.h>
#include <interpret_chp/export_dot.h>

#include "helpers.h"
#include "dot.h"

using namespace std;
using namespace test;

// Helper function to parse a string into an HSE graph
chp::graph load_chp_string(string input) {
	// Set up tokenizer
	tokenizer tokens;
	tokens.register_token<parse::block_comment>(false);
	tokens.register_token<parse::line_comment>(false);
	parse_chp::composition::register_syntax(tokens);

	tokens.insert("string_input", input, nullptr);

	chp::graph g;

	// Parse input
	tokens.increment(false);
	tokens.expect<parse_chp::composition>();
	if (tokens.decrement(__FILE__, __LINE__)) {
		parse_chp::composition syntax(tokens);
		chp::import_chp(g, syntax, &tokens, true);
	}

	return g;
}

// Test basic sequence import (a+; b+; a-; b-)
TEST(ChpImport, Sequence) {
	chp::graph g = load_chp_string("a+; b+; c-; d-");

	gvdot::render("sequence.png", chp::export_graph(g, true).to_string());

	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 4);
	EXPECT_EQ(g.transitions.size(), 4u);
	EXPECT_EQ(g.places.size(), 4u);

	auto True = arithmetic::Operand::boolOf(true);
	auto False = arithmetic::Operand::boolOf(false);
	int ai = g.netIndex("a");
	int bi = g.netIndex("b");
	int ci = g.netIndex("c");
	int di = g.netIndex("d");
	EXPECT_GE(ai, 0);
	EXPECT_GE(bi, 0);
	EXPECT_GE(ci, 0);
	EXPECT_GE(di, 0);
	auto a = arithmetic::Operand::varOf(ai);
	auto b = arithmetic::Operand::varOf(bi);
	auto c = arithmetic::Operand::varOf(ci);
	auto d = arithmetic::Operand::varOf(di);

	vector<petri::iterator> a1 = findRule(g, True, {{arithmetic::Action(a.index, True)}});
	vector<petri::iterator> b1 = findRule(g, True, {{arithmetic::Action(b.index, True)}});
	vector<petri::iterator> a0 = findRule(g, True, {{arithmetic::Action(c.index, False)}});
	vector<petri::iterator> b0 = findRule(g, True, {{arithmetic::Action(d.index, False)}});
	
	ASSERT_EQ(a1.size(), 1u);
	ASSERT_EQ(b1.size(), 1u);
	ASSERT_EQ(a0.size(), 1u);
	ASSERT_EQ(b0.size(), 1u);

	EXPECT_TRUE(g.is_sequence(a1[0], b1[0]));
	EXPECT_TRUE(g.is_sequence(b1[0], a0[0]));
	EXPECT_TRUE(g.is_sequence(a0[0], b0[0]));
}

// Test parallel composition import ((a+, b+); (a-, b-))
TEST(ChpImport, Parallel) {
	chp::graph g = load_chp_string("(a+, b+); (a-, b-)");
	
	gvdot::render("parallel.png", chp::export_graph(g, true).to_string());
	
	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 2);
	EXPECT_EQ(g.transitions.size(), 5u);
	
	auto True = arithmetic::Operand::boolOf(true);
	auto False = arithmetic::Operand::boolOf(false);
	int ai = g.netIndex("a");
	int bi = g.netIndex("b");
	EXPECT_GE(ai, 0);
	EXPECT_GE(bi, 0);
	auto a = arithmetic::Operand::varOf(ai);
	auto b = arithmetic::Operand::varOf(bi);

	vector<petri::iterator> a1 = findRule(g, True, {{arithmetic::Action(a.index, True)}});
	vector<petri::iterator> b1 = findRule(g, True, {{arithmetic::Action(b.index, True)}});
	vector<petri::iterator> a0 = findRule(g, True, {{arithmetic::Action(a.index, False)}});
	vector<petri::iterator> b0 = findRule(g, True, {{arithmetic::Action(b.index, False)}});
	vector<petri::iterator> sp = findRule(g, True, true);
	
	ASSERT_EQ(a1.size(), 1u);
	ASSERT_EQ(b1.size(), 1u);
	ASSERT_EQ(a0.size(), 1u);
	ASSERT_EQ(b0.size(), 1u);
	ASSERT_EQ(sp.size(), 1u);

	// Verify parallel structure - a+ and b+ should be concurrent
	EXPECT_TRUE(g.is_parallel(a1[0], b1[0]));
	EXPECT_TRUE(g.is_parallel(a0[0], b0[0]));
	
	// Verify sequencing - first parallel group completes before second group
	EXPECT_TRUE(g.is_sequence(a1[0], sp[0]));
	EXPECT_TRUE(g.is_sequence(sp[0], a0[0]));
	EXPECT_TRUE(g.is_sequence(b1[0], sp[0]));
	EXPECT_TRUE(g.is_sequence(sp[0], b0[0]));
}

// Test selection import ([c -> a+; a- [] ~c -> b+; b-])
TEST(ChpImport, Selection) {
	chp::graph g = load_chp_string("[c -> a+; a- [] ~c -> b+; b-]");
	
	gvdot::render("selection.png", chp::export_graph(g, true).to_string());

	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 3);  // a, b, and c
	EXPECT_EQ(g.transitions.size(), 6u);
	
	auto True = arithmetic::Operand::boolOf(true);
	auto False = arithmetic::Operand::boolOf(false);
	int ai = g.netIndex("a");
	int bi = g.netIndex("b");
	int ci = g.netIndex("c");
	EXPECT_GE(ai, 0);
	EXPECT_GE(bi, 0);
	EXPECT_GE(ci, 0);
	auto a = arithmetic::Operand::varOf(ai);
	auto b = arithmetic::Operand::varOf(bi);
	auto c = arithmetic::Operand::varOf(ci);

	vector<petri::iterator> a1 = findRule(g, True, {{arithmetic::Action(a.index, True)}});
	vector<petri::iterator> b1 = findRule(g, True, {{arithmetic::Action(b.index, True)}});
	vector<petri::iterator> a0 = findRule(g, True, {{arithmetic::Action(a.index, False)}});
	vector<petri::iterator> b0 = findRule(g, True, {{arithmetic::Action(b.index, False)}});
	vector<petri::iterator> c1 = findRule(g, c, true);
	vector<petri::iterator> c0 = findRule(g, ~c, true);

	ASSERT_EQ(a1.size(), 1u);
	ASSERT_EQ(b1.size(), 1u);
	ASSERT_EQ(a0.size(), 1u);
	ASSERT_EQ(b0.size(), 1u);
	ASSERT_EQ(c1.size(), 1u);
	ASSERT_EQ(c0.size(), 1u);
	
	// Verify selection structure (no path between a+ and b+)
	EXPECT_TRUE(g.is_choice(c1[0], c0[0]));
	EXPECT_TRUE(g.is_choice(a1[0], b1[0]));
	EXPECT_TRUE(g.is_choice(a0[0], b0[0]));
	
	// Verify each branch internal sequencing
	EXPECT_TRUE(g.is_sequence(c1[0], a1[0]));
	EXPECT_TRUE(g.is_sequence(a1[0], a0[0]));

	EXPECT_TRUE(g.is_sequence(c0[0], b1[0]));
	EXPECT_TRUE(g.is_sequence(b1[0], b0[0]));
}

// Test loop import (*[a+; b+; a-; b-])
TEST(ChpImport, Loop) {
	chp::graph g = load_chp_string("*[a+; b+; a-; b-]");
	
	gvdot::render("loop.png", chp::export_graph(g, true).to_string());
	
	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 2);
	EXPECT_EQ(g.transitions.size(), 4u);
	
	auto True = arithmetic::Operand::boolOf(true);
	auto False = arithmetic::Operand::boolOf(false);
	int ai = g.netIndex("a");
	int bi = g.netIndex("b");
	EXPECT_GE(ai, 0);
	EXPECT_GE(bi, 0);
	auto a = arithmetic::Operand::varOf(ai);
	auto b = arithmetic::Operand::varOf(bi);

	vector<petri::iterator> a1 = findRule(g, True, {{arithmetic::Action(a.index, True)}});
	vector<petri::iterator> b1 = findRule(g, True, {{arithmetic::Action(b.index, True)}});
	vector<petri::iterator> a0 = findRule(g, True, {{arithmetic::Action(a.index, False)}});
	vector<petri::iterator> b0 = findRule(g, True, {{arithmetic::Action(b.index, False)}});
	
	ASSERT_EQ(a1.size(), 1u);
	ASSERT_EQ(b1.size(), 1u);
	ASSERT_EQ(a0.size(), 1u);
	ASSERT_EQ(b0.size(), 1u);
	
	// Verify cycle: should be able to go from any transition back to itself
	EXPECT_TRUE(g.is_sequence(a1[0], b1[0]));
	EXPECT_TRUE(g.is_sequence(b1[0], a0[0]));
	EXPECT_TRUE(g.is_sequence(a0[0], b0[0]));
	EXPECT_TRUE(g.is_sequence(b0[0], a1[0]));
}

// Test more complex expressions with composition
TEST(ChpImport, ComplexComposition) {
	chp::graph g = load_chp_string("(a+; b+) || (c+; d+)");
	
	gvdot::render("complex.png", chp::export_graph(g, true).to_string());
	
	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 4);  // a, b, c, d
	EXPECT_GE(g.transitions.size(), 4u);
	
	auto True = arithmetic::Operand::boolOf(true);
	auto False = arithmetic::Operand::boolOf(false);
	int ai = g.netIndex("a");
	int bi = g.netIndex("b");
	int ci = g.netIndex("c");
	int di = g.netIndex("d");
	EXPECT_GE(ai, 0);
	EXPECT_GE(bi, 0);
	EXPECT_GE(ci, 0);
	EXPECT_GE(di, 0);
	auto a = arithmetic::Operand::varOf(ai);
	auto b = arithmetic::Operand::varOf(bi);
	auto c = arithmetic::Operand::varOf(ci);
	auto d = arithmetic::Operand::varOf(di);

	vector<petri::iterator> a1 = findRule(g, True, {{arithmetic::Action(a.index, True)}});
	vector<petri::iterator> b1 = findRule(g, True, {{arithmetic::Action(b.index, True)}});
	vector<petri::iterator> c1 = findRule(g, True, {{arithmetic::Action(c.index, True)}});
	vector<petri::iterator> d1 = findRule(g, True, {{arithmetic::Action(d.index, True)}});
	
	ASSERT_EQ(a1.size(), 1u);
	ASSERT_EQ(b1.size(), 1u);
	ASSERT_EQ(c1.size(), 1u);
	ASSERT_EQ(d1.size(), 1u);
	
	// Verify sequence within each composition
	EXPECT_TRUE(g.is_sequence(a1[0], b1[0]));
	EXPECT_TRUE(g.is_sequence(c1[0], d1[0]));
	
	// Verify independence between compositions
	EXPECT_TRUE(g.is_parallel(a1[0], c1[0]));
	EXPECT_TRUE(g.is_parallel(c1[0], a1[0]));
	EXPECT_TRUE(g.is_parallel(b1[0], d1[0]));
	EXPECT_TRUE(g.is_parallel(d1[0], b1[0]));
}

// Test nested control structures
TEST(ChpImport, NestedControls) {
	chp::graph g = load_chp_string("*[[a -> b+; b- [] ~a -> c+; (d+, e+); c-; (d-, e-)]]");
	
	gvdot::render("nested.png", chp::export_graph(g, true).to_string());
	
	// Verify the graph structure
	EXPECT_GT(g.netCount(), 4);  // a, b, c, d, e
	EXPECT_GT(g.transitions.size(), 10u);  // At least b+, b-, c+, c-, d+/-, e+/-
	
	auto True = arithmetic::Operand::boolOf(true);
	auto False = arithmetic::Operand::boolOf(false);
	int ai = g.netIndex("a");
	int bi = g.netIndex("b");
	int ci = g.netIndex("c");
	int di = g.netIndex("d");
	int ei = g.netIndex("e");
	EXPECT_GE(ai, 0);
	EXPECT_GE(bi, 0);
	EXPECT_GE(ci, 0);
	EXPECT_GE(di, 0);
	EXPECT_GE(ei, 0);
	auto a = arithmetic::Operand::varOf(ai);
	auto b = arithmetic::Operand::varOf(bi);
	auto c = arithmetic::Operand::varOf(ci);
	auto d = arithmetic::Operand::varOf(di);
	auto e = arithmetic::Operand::varOf(ei);

	vector<petri::iterator> b1 = findRule(g, True, {{arithmetic::Action(b.index, True)}});
	vector<petri::iterator> b0 = findRule(g, True, {{arithmetic::Action(b.index, False)}});
	vector<petri::iterator> c1 = findRule(g, True, {{arithmetic::Action(c.index, True)}});
	vector<petri::iterator> c0 = findRule(g, True, {{arithmetic::Action(c.index, False)}});
	vector<petri::iterator> d1 = findRule(g, True, {{arithmetic::Action(d.index, True)}});
	vector<petri::iterator> d0 = findRule(g, True, {{arithmetic::Action(d.index, False)}});
	vector<petri::iterator> e1 = findRule(g, True, {{arithmetic::Action(e.index, True)}});
	vector<petri::iterator> e0 = findRule(g, True, {{arithmetic::Action(e.index, False)}});
	vector<petri::iterator> a0 = findRule(g, ~a, true);
	vector<petri::iterator> a1 = findRule(g, a, true);
	vector<petri::iterator> sp = findRule(g, True, true);
	
	ASSERT_EQ(b1.size(), 1u);
	ASSERT_EQ(b0.size(), 1u);
	ASSERT_EQ(c1.size(), 1u);
	ASSERT_EQ(c0.size(), 1u);
	ASSERT_EQ(d1.size(), 1u);
	ASSERT_EQ(d0.size(), 1u);
	ASSERT_EQ(e1.size(), 1u);
	ASSERT_EQ(e0.size(), 1u);
	ASSERT_EQ(a0.size(), 1u);
	ASSERT_EQ(a1.size(), 1u);
	ASSERT_EQ(sp.size(), 1u);
	
	// Verify loops - all transitions should be part of a cycle
	EXPECT_TRUE(g.is_sequence(b1[0], b0[0]));
	EXPECT_TRUE(g.is_sequence(c1[0], d1[0]));
	EXPECT_TRUE(g.is_sequence(c1[0], e1[0]));
	EXPECT_TRUE(g.is_sequence(d1[0], c0[0]));
	EXPECT_TRUE(g.is_sequence(e1[0], c0[0]));
	EXPECT_TRUE(g.is_sequence(c0[0], d0[0]));
	EXPECT_TRUE(g.is_sequence(c0[0], e0[0]));

	EXPECT_TRUE(g.is_sequence(d0[0], sp[0]));
	EXPECT_TRUE(g.is_sequence(e0[0], sp[0]));

	EXPECT_TRUE(g.is_sequence(sp[0], a0[0]));
	EXPECT_TRUE(g.is_sequence(b0[0], a1[0]));

	EXPECT_TRUE(g.is_sequence(a1[0], b1[0]));
	EXPECT_TRUE(g.is_sequence(a0[0], c1[0]));
}

TEST(ChpImport, Counter) {
	chp::graph g = load_chp_string(R"(
R.i-,R.d-,R0-,R1-,Rz-,L.z-,L.n-,v0-,v1-,vz+; [R.z&~R.n&~L.i&~L.d];
*[[  v1 & (R.z | R.n) & L.i -> R.i+
  [] (v0 | vz) & (R.z | R.n) & L.d -> R.d+
  [] (v0 | vz) & L.i -> R1+
  [] v1 & R.n & L.d -> R0+
  [] v1 & R.z & L.d -> Rz+
  ]; L.z-, L.n-;
  [~L.i & ~L.d];
  [  Rz -> vz+; v0-,v1-
  [] R.i | R0 -> v0+; v1-,vz-
  [] R.d | R1 -> v1+; v0-,vz-
  ];
	(
	  [~v0 & ~v1]; Rz- ||
	  [~v1 & ~vz]; R0- ||
		[~v1 & ~vz & ~R.z & ~R.n]; R.i- ||
		[~v0 & ~vz]; R1- ||
		[~v0 & ~vz & ~R.z & ~R.n]; R.d-
  ); @
  [  ~v0 & ~v1 -> L.z+
  [] ~vz -> L.n+
  ]
 ] ||

(L.i-,L.d-;[~L.z&~L.n]; *[[L.z | L.n]; [1->L.i+:1->L.d+]; [~L.z&~L.n]; L.i-,L.d-] ||
R.z+,R.n-;[~R.i&~R.d]; *[[R.i | R.d]; R.z-,R.n-; [~R.i&~R.d]; [1->R.z+:1->R.n+]])'1
)");

	g.post_process();
	
	gvdot::render("counter.png", chp::export_graph(g, true).to_string());
}


