#include <common/standard.h>
#include <gtest/gtest.h>

#include <chp/graph.h>
#include <parse/parse.h>
#include <parse/tokenizer.h>
#include <parse/default/block_comment.h>
#include <parse/default/line_comment.h>
#include <parse_cog/composition.h>
#include <parse_cog/branch.h>
#include <parse_cog/control.h>
#include <parse_cog/factory.h>
#include <interpret_chp/import_cog.h>
#include <interpret_chp/export_dot.h>

#include "helpers.h"

using namespace std;
using namespace test;

// Helper function to parse a string into an HSE graph via COG
/*chp::graph load_cog_string(string input) {
	// Set up tokenizer
	tokenizer tokens;
	tokens.register_token<parse::block_comment>(false);
	tokens.register_token<parse::line_comment>(false);
	parse_cog::composition::register_syntax(tokens);
	
	tokens.insert("string_input", input, nullptr);
	
	chp::graph g;

	// Parse input
	tokens.increment(false);
	tokens.expect<parse_cog::composition>();
	if (tokens.decrement(__FILE__, __LINE__)) {
		parse_cog::composition syntax(tokens);
		chp::import_chp(g, syntax, &tokens, true);
	}

	return g;
}

// Test basic sequence import 
TEST(CogImport, BasicSequence) {
	chp::graph g = load_cog_string(R"(
		region 1 {
			a+
			b+
			a-
			b-
		}
	)");
	
	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 2);  // a and b
	EXPECT_GE(g.transitions.size(), 4u);  // a+, b+, a-, b-
	
	int a = g.netIndex("a'1");
	int b = g.netIndex("b'1");
	EXPECT_GE(a, 0);
	EXPECT_GE(b, 0);
   
	vector<petri::iterator> a1 = findRule(g, 1, boolean::cover(a, 1));
	vector<petri::iterator> b1 = findRule(g, 1, boolean::cover(b, 1));
	vector<petri::iterator> a0 = findRule(g, 1, boolean::cover(a, 0));
	vector<petri::iterator> b0 = findRule(g, 1, boolean::cover(b, 0));
	
	ASSERT_FALSE(a1.empty());
	ASSERT_FALSE(b1.empty());
	ASSERT_FALSE(a0.empty());
	ASSERT_FALSE(b0.empty());
	
	// Verify sequence: a+ -> b+ -> a- -> b-
	EXPECT_TRUE(g.is_sequence(a1[0], b1[0]));
	EXPECT_TRUE(g.is_sequence(b1[0], a0[0]));
	EXPECT_TRUE(g.is_sequence(a0[0], b0[0]));
}

// Test parallel composition
TEST(CogImport, ParallelComposition) {
	chp::graph g = load_cog_string(R"(
		{
			a+
			b+
		} and {
			c+
			d+
		}
	)");
	
	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 4);  // a, b, c, d
	EXPECT_GE(g.transitions.size(), 4u);  // a+; b+ || c+; d+
	
	int a = g.netIndex("a");
	int b = g.netIndex("b");
	int c = g.netIndex("c");
	int d = g.netIndex("d");
	EXPECT_GE(a, 0);
	EXPECT_GE(b, 0);
	EXPECT_GE(c, 0);
	EXPECT_GE(d, 0);
	
	vector<petri::iterator> a1 = findRule(g, 1, boolean::cover(a, 1));
	vector<petri::iterator> b1 = findRule(g, 1, boolean::cover(b, 1));
	vector<petri::iterator> c1 = findRule(g, 1, boolean::cover(c, 1));
	vector<petri::iterator> d1 = findRule(g, 1, boolean::cover(d, 1));
	
	ASSERT_FALSE(a1.empty());
	ASSERT_FALSE(b1.empty());
	ASSERT_FALSE(c1.empty());
	ASSERT_FALSE(d1.empty());
	
	// Verify sequencing within each branch
	EXPECT_TRUE(g.is_sequence(a1[0], b1[0]));
	EXPECT_TRUE(g.is_sequence(c1[0], d1[0]));
	
	// Verify parallelism between branches
	EXPECT_TRUE(g.is_parallel(a1[0], c1[0]));
	EXPECT_TRUE(g.is_parallel(a1[0], d1[0]));
	EXPECT_TRUE(g.is_parallel(b1[0], c1[0]));
	EXPECT_TRUE(g.is_parallel(b1[0], d1[0]));
}

// Test conditional waiting
TEST(CogImport, ConditionalWaiting) {
	chp::graph g = load_cog_string(R"(
		{
			await a {
				b+
			} xor await ~a {
				c+
			}
		}
	)");
	
	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 3);  // a, b, c
	EXPECT_GE(g.transitions.size(), 4u);  // [a]; b+; [~a]; c+
	
	int a = g.netIndex("a");
	int b = g.netIndex("b");
	int c = g.netIndex("c");
	EXPECT_GE(a, 0);
	EXPECT_GE(b, 0);
	EXPECT_GE(c, 0);
	
	vector<petri::iterator> b1 = findRule(g, 1, boolean::cover(b, 1));
	vector<petri::iterator> c1 = findRule(g, 1, boolean::cover(c, 1));
	vector<petri::iterator> a1 = findRule(g, boolean::cover(a, 1), 1);
	vector<petri::iterator> a0 = findRule(g, boolean::cover(a, 0), 1);
	
	ASSERT_FALSE(b1.empty());
	ASSERT_FALSE(c1.empty());
	ASSERT_FALSE(a1.empty());
	ASSERT_FALSE(a0.empty());
	
		EXPECT_TRUE(g.is_sequence(a1[0], b1[0]));
		EXPECT_TRUE(g.is_sequence(a0[0], c1[0]));
		EXPECT_TRUE(g.is_choice(a1[0], a0[0]));
		EXPECT_TRUE(g.is_choice(a1[0], c1[0]));
		EXPECT_TRUE(g.is_choice(a0[0], b1[0]));
		EXPECT_TRUE(g.is_choice(c1[0], b1[0]));
}

// Test while loop
TEST(CogImport, WhileLoop) {
	chp::graph g = load_cog_string(R"(
		region 1 {
			while {
				a+
				b+
				a-
				b-
			}
		}
	)");
	
	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 2);  // a and b
	EXPECT_GE(g.transitions.size(), 4u);  // a+, b+, a-, b-
	
	int a = g.netIndex("a'1");
	int b = g.netIndex("b'1");
	EXPECT_GE(a, 0);
	EXPECT_GE(b, 0);
	
	vector<petri::iterator> a1 = findRule(g, 1, boolean::cover(a, 1));
	vector<petri::iterator> b1 = findRule(g, 1, boolean::cover(b, 1));
	vector<petri::iterator> a0 = findRule(g, 1, boolean::cover(a, 0));
	vector<petri::iterator> b0 = findRule(g, 1, boolean::cover(b, 0));
	
	ASSERT_FALSE(a1.empty());
	ASSERT_FALSE(b1.empty());
	ASSERT_FALSE(a0.empty());
	ASSERT_FALSE(b0.empty());
	
	// Verify cycle: should be able to go from any transition back to itself
	EXPECT_TRUE(g.is_sequence(b0[0], a1[0]));
	EXPECT_TRUE(g.is_sequence(a1[0], b1[0]));
	EXPECT_TRUE(g.is_sequence(b1[0], a0[0]));
	EXPECT_TRUE(g.is_sequence(a0[0], b0[0]));
}

// Test COG-specific 'await' construct
TEST(CogImport, AwaitConstruct) {
	chp::graph g = load_cog_string(R"(
		region 1 {
			a+
			await b
			a-
			await ~b
		}
	)");
	
	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 2);  // a and b
	EXPECT_GE(g.transitions.size(), 4u);  // a+; [b]; a-; [~b]
	
	int a = g.netIndex("a'1");
	int b = g.netIndex("b'1");
	EXPECT_GE(a, 0);
	EXPECT_GE(b, 0);
	
	vector<petri::iterator> a1 = findRule(g, 1, boolean::cover(a, 1));
	vector<petri::iterator> a0 = findRule(g, 1, boolean::cover(a, 0));
	vector<petri::iterator> b1 = findRule(g, boolean::cover(b, 1), 1);
	vector<petri::iterator> b0 = findRule(g, boolean::cover(b, 0), 1);
	
	ASSERT_EQ(a1.size(), 1u);
	ASSERT_EQ(a0.size(), 1u);
	ASSERT_EQ(b1.size(), 1u);
	ASSERT_EQ(b0.size(), 1u);
	
	// a+ and a- should be sequenced
	EXPECT_TRUE(g.is_sequence(a1[0], b1[0]));
	EXPECT_TRUE(g.is_sequence(b1[0], a0[0]));
	EXPECT_TRUE(g.is_sequence(a0[0], b0[0]));
}

// Test WCHB buffer from example
TEST(CogImport, WCHB1bBuffer) {
	chp::graph g = load_cog_string(R"(
		region 1 {
			L.f- and L.t-
			await L.e
			while {
				L.f+ xor L.t+
				await ~L.e
				L.f- and L.t-
				await L.e
			}
		} and {
			L.e+ and R.f- and R.t-
			await R.e & ~L.f & ~L.t
			while {
				await R.e & L.f {
					R.f+
				} or await R.e & L.t {
					R.t+
				}
				L.e-
				await ~R.e & ~L.f & ~L.t
				R.f- and R.t-
				L.e+
			}
		} and region 1 {
			R.e+
			await ~R.f & ~R.t
			while {
				await R.f | R.t
				R.e-
				await ~R.f & ~R.t
				R.e+
			}
		}
	)");
	
	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 12);  // L.e, L.f, L.t, R.e, R.f, R.t
	EXPECT_GE(g.transitions.size(), 28u);  // Various transitions for each signal
	
	// Check for key signals
	int le = g.netIndex("L.e");
	int lf = g.netIndex("L.f");
	int lt = g.netIndex("L.t");
	int re = g.netIndex("R.e");
	int rf = g.netIndex("R.f");
	int rt = g.netIndex("R.t");
	
	EXPECT_GE(le, 0);
	EXPECT_GE(lf, 0);
	EXPECT_GE(lt, 0);
	EXPECT_GE(re, 0);
	EXPECT_GE(rf, 0);
	EXPECT_GE(rt, 0);
	
	// Check for transitions for all signals
	vector<petri::iterator> le1 = findRule(g, 1, boolean::cover(le, 1));
	vector<petri::iterator> le0 = findRule(g, 1, boolean::cover(le, 0));
	vector<petri::iterator> lf1 = findRule(g, boolean::cover(re, 1) & boolean::cover(lf, 1), 1);
	vector<petri::iterator> lt1 = findRule(g, boolean::cover(re, 1) & boolean::cover(lt, 1), 1);
	vector<petri::iterator> lft0 = findRule(g, boolean::cover(re, 0) & boolean::cover(lf, 0) & boolean::cover(lt, 0), 1);
	vector<petri::iterator> rf1 = findRule(g, 1, boolean::cover(rf, 1));
	vector<petri::iterator> rf0 = findRule(g, 1, boolean::cover(rf, 0));
	vector<petri::iterator> rt1 = findRule(g, 1, boolean::cover(rt, 1));
	vector<petri::iterator> rt0 = findRule(g, 1, boolean::cover(rt, 0));
	
	ASSERT_EQ(le1.size(), 2u);
	ASSERT_EQ(le0.size(), 1u);
	ASSERT_EQ(lf1.size(), 1u);
	ASSERT_EQ(lt1.size(), 1u);
	ASSERT_EQ(lft0.size(), 1u);
	ASSERT_EQ(rf1.size(), 1u);
	ASSERT_EQ(rf0.size(), 2u);
	ASSERT_EQ(rt1.size(), 1u);
	ASSERT_EQ(rt0.size(), 2u);
	
	EXPECT_TRUE(g.is_sequence(lf1[0], rf1[0]));
	EXPECT_TRUE(g.is_sequence(lt1[0], rt1[0]));
	EXPECT_TRUE(g.is_choice(lf1[0], lt1[0]));
	EXPECT_TRUE(g.is_sequence(le0[0], lft0[0]));
	EXPECT_TRUE(g.is_sequence(lft0[0], rf0[1]));
	EXPECT_TRUE(g.is_sequence(lft0[0], rt0[1]));
	EXPECT_TRUE(g.is_parallel(rf0[1], rt0[1]));
	EXPECT_TRUE(g.is_sequence(rf0[1], le1[1]));
	EXPECT_TRUE(g.is_sequence(rt0[1], le1[1]));
}*/

