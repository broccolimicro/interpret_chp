#include <common/standard.h>
#include <gtest/gtest.h>

#include <interpret_chp/export_cog.h>
#include <interpret_chp/export_dot.h>

#include <petri/synthesize.h>

#include "helpers.h"
#include "dot.h"

using namespace std;
using namespace test;

void expectEq(std::string expected, std::string actual) {
	auto diff = lineDiff(actual, expected);
	EXPECT_TRUE(isMatch(diff)) << diff;
}

void testCogExport(std::string source, std::string target, bool debug=false) {
	const ::testing::TestInfo* const testInfo =
			::testing::UnitTest::GetInstance()->current_test_info();
	std::string testSuiteName = testInfo->test_suite_name();
	std::string testName = testInfo->name();

	chp::graph g = load_cog_string(source);

	if (debug) {
		gvdot::render(testSuiteName + "_" + testName + ".png", chp::export_graph(g, true).to_string());
	}

	petri::graph<chp::place, controlflow::Tree<chp::transition, chp::place>, chp::state> result;
	EXPECT_TRUE(petri::graph_to_tree(result, g)) << "resulting graph not properly nested";
	for (size_t i = 0; i < result.transitions.size(); i++) {
		if (not result.transitions.is_valid(i)) continue;

		expectEq(target, parse_cog::export_cog(result.transitions[i], g).to_string(""));
	}
}

TEST(CogExport, Copy) {
	std::string source = R"(until {
	x=L.recv()
	A.send(x) and B.send(x)
})";

	testCogExport(source, source);
}

TEST(CogExport, Loops1) {
	std::string source = R"(until G {
	A.send(L.recv())
} and until L {
	B.send(G.recv())
} or await C {
	until {
		C.recv()
	}
} xor until {
	await C {
		C.recv()
	}
})";

std::string target = R"(until G {
	A.send(L.recv())
} and until L {
	B.send(G.recv())
} xor await C {
	until {
		C.recv()
	}
} xor until {
	await C
	C.recv()
})";

	testCogExport(source, target);
}

TEST(CogExport, Loops2) {
	std::string source = R"(when L {
	A.send(L.recv())
} and when G {
	B.send(G.recv())
} or await C {
	when {
		C.recv()
	}
} xor when {
	await C {
		C.recv()
	}
})";

	std::string target = R"(until {
	await L
	A.send(L.recv())
} and {
	xuntil C {
		await G {
			B.send(G.recv())
		} xor await C {
			C.recv()
		}
	}
	until {
		C.recv()
	}
})";

	testCogExport(source, target);
}

TEST(CogExport, Loops3) {
	std::string source = R"(until {
	A.send(L.recv())
} and {
	until C {
		await G {
			B.send(G.recv())
		} or await C {
			C.recv()
		}
	}
	until {
		C.recv()
	}
})";

	testCogExport(source, source);
}

/*TEST(CogExport, Add) {
	chp::graph g = load_cog_string(R"(
while {
	S.send(A.recv() + B.recv())
}
)");

	gvdot::render("add.png", chp::export_graph(g, true).to_string());
	
	EXPECT_EQ(g.netCount(), 3);
	EXPECT_GE(g.transitions.size(), 1u);
	
	int vA = g.netIndex("A");
	int vB = g.netIndex("B");
	int vS = g.netIndex("S");
	EXPECT_GE(vA, 0);
	EXPECT_GE(vB, 0);
	EXPECT_GE(vS, 0);

	auto True = arithmetic::Operand(true);
	auto False = arithmetic::Operand(false);

	auto A = arithmetic::Operand::varOf(vA);
	auto B = arithmetic::Operand::varOf(vB);
	auto S = arithmetic::Operand::varOf(vS);
}

TEST(CogExport, Split) {
	chp::graph g = load_cog_string(R"(
while {
	x = L.recv() and c = C.recv()
	if c == 0 {
		A.send(x)
	} or if c == 1 {
		B.send(x)
	}
}
)");

	gvdot::render("split.png", chp::export_graph(g, true).to_string());
	
	EXPECT_EQ(g.netCount(), 6);
	EXPECT_GE(g.transitions.size(), 6u);
	
	int vx = g.netIndex("x");
	int vL = g.netIndex("L");
	int vC = g.netIndex("C");
	int vc = g.netIndex("c");
	int vA = g.netIndex("A");
	int vB = g.netIndex("B");
	EXPECT_GE(vx, 0);
	EXPECT_GE(vL, 0);
	EXPECT_GE(vC, 0);
	EXPECT_GE(vc, 0);
	EXPECT_GE(vA, 0);
	EXPECT_GE(vB, 0);

	auto True = arithmetic::Operand(true);
	auto False = arithmetic::Operand(false);

	auto x = arithmetic::Expression::varOf(vx);
	auto L = arithmetic::Expression::varOf(vL);
	auto C = arithmetic::Expression::varOf(vC);
	auto c = arithmetic::Expression::varOf(vc);
	auto A = arithmetic::Expression::varOf(vA);
	auto B = arithmetic::Expression::varOf(vB);

	vector<petri::iterator> c0 = findRule(g, isTrue(c==0), true);
	vector<petri::iterator> c1 = findRule(g, isTrue(c==1), true);
	
	ASSERT_FALSE(c0.empty());
	ASSERT_FALSE(c1.empty());
}

TEST(CogExport, Merge) {
	chp::graph g = load_cog_string(R"(
while {
	c = C.recv()
	if c == 0 {
		x = A.recv()
	} or if c == 1 {
		x = B.recv()
	}
	R.send(x)
}
)");

	gvdot::render("merge.png", chp::export_graph(g, true).to_string());
	
	EXPECT_EQ(g.netCount(), 6);
	EXPECT_GE(g.transitions.size(), 6u);
	
	int vx = g.netIndex("x");
	int vA = g.netIndex("A");
	int vB = g.netIndex("B");
	int vC = g.netIndex("C");
	int vc = g.netIndex("c");
	int vR = g.netIndex("R");
	EXPECT_GE(vx, 0);
	EXPECT_GE(vA, 0);
	EXPECT_GE(vB, 0);
	EXPECT_GE(vC, 0);
	EXPECT_GE(vc, 0);
	EXPECT_GE(vR, 0);

	auto True = arithmetic::Operand(true);
	auto False = arithmetic::Operand(false);

	auto x = arithmetic::Expression::varOf(vx);
	auto A = arithmetic::Expression::varOf(vA);
	auto B = arithmetic::Expression::varOf(vB);
	auto C = arithmetic::Expression::varOf(vC);
	auto c = arithmetic::Expression::varOf(vc);
	auto R = arithmetic::Expression::varOf(vR);

	vector<petri::iterator> c0 = findRule(g, isTrue(c==0), true);
	vector<petri::iterator> c1 = findRule(g, isTrue(c==1), true);
	
	ASSERT_FALSE(c0.empty());
	ASSERT_FALSE(c1.empty());
}

TEST(CogExport, Counter) {
	chp::graph g = load_cog_string(R"(
v = 0
z = true
while {
	Lz.send(z)
	l = Lc.recv()
	if l == 0 && v == 0 {
		v = 1
		z = Rz.recv()
		Rc.send(0)
	} or if l == 0 && v == 1 {
		v = 0
		z = false
	} or if l == 1 && v == 0 {
		v = 1
		z = false
	} or if l == 1 && v == 1 {
		v = 0
		z = Rz.recv()
		Rc.send(1)
	}
}
)");

	gvdot::render("cog_counter.png", chp::export_graph(g, true).to_string());
}*/
