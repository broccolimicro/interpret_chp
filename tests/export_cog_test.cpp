#include <gtest/gtest.h>
#include <common/standard.h>
#include <common/mapping.h>

#include <interpret_chp/export_cog.h>
#include <interpret_chp/export_dot.h>

#include <flow/func.h>
#include <petri/synthesize.h>

#include "helpers.h"
#include "dot.h"

using namespace std;
using namespace test;
using arithmetic::Expression;

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
	if (debug) {
		result.print();
	}
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

TEST(CogExport, Merge) {
	std::string source = R"""(while {
	c = C.recv()
	if c == 0 {
		x = A.recv()
	} or if c == 1 {
		x = B.recv()
	}
	R.send(x)
})""";

	std::string target = R"""(until {
	c=C.recv()
	if c==0 {
		x=A.recv()
	} or if c==1 {
		x=B.recv()
	}
	R.send(x)
})""";

	testCogExport(source, target);
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

const size_t WIDTH = 4;

TEST(FlowToFlowExport, Source) {
	using namespace flow;
	Func func;
	func.name = "source";
	Operand R = func.pushNet("R", Type(Type::TypeName::FIXED, WIDTH), flow::Net::OUT);

	size_t branch0 = func.pushCond(Expression::boolOf(true));
	func.conds[branch0].req(R, Expression::intOf(1));  //TODO: send random int?

	cout << parse_cog::export_func(func).to_string("") << endl;
}

TEST(FlowToFlowExport, Sink) {
	using namespace flow;
	Func func;
	func.name = "sink";
	Operand L = func.pushNet("L", Type(Type::TypeName::FIXED, WIDTH), flow::Net::IN);

	size_t branch0 = func.pushCond(Expression::boolOf(true));
	func.conds[branch0].ack(L);

	cout << parse_cog::export_func(func).to_string("") << endl;
}

TEST(FlowToFlowExport, Buffer) {
	using namespace flow;
	Func func;
	func.name = "buffer";
	Operand L = func.pushNet("L", Type(Type::TypeName::FIXED, WIDTH), flow::Net::IN);
	Operand R = func.pushNet("R", Type(Type::TypeName::FIXED, WIDTH), flow::Net::OUT);
	Expression exprL(L);

	size_t branch0 = func.pushCond(Expression::boolOf(true));
	func.conds[branch0].req(R, exprL);
	func.conds[branch0].ack(L);

	cout << parse_cog::export_func(func).to_string("") << endl;
}

TEST(FlowToFlowExport, Copy) {
	using namespace flow;
	Func func;
	func.name = "copy";
	Operand L = func.pushNet("L", Type(Type::TypeName::FIXED, WIDTH), flow::Net::IN);
	Operand R0 = func.pushNet("R0", Type(Type::TypeName::FIXED, WIDTH), flow::Net::OUT);
	Operand R1 = func.pushNet("R1", Type(Type::TypeName::FIXED, WIDTH), flow::Net::OUT);
	Expression exprL(L);

	size_t branch0 = func.pushCond(Expression::boolOf(true));
	func.conds[branch0].req(R0, exprL);
	func.conds[branch0].req(R1, exprL);
	func.conds[branch0].ack(L);

	cout << parse_cog::export_func(func).to_string("") << endl;
}

TEST(FlowToFlowExport, Func) {
	using namespace flow;
	Func func;
	func.name = "func";
	Operand L0 = func.pushNet("L0", Type(Type::TypeName::FIXED, WIDTH), flow::Net::IN);
	Operand L1 = func.pushNet("L1", Type(Type::TypeName::FIXED, WIDTH), flow::Net::IN);
	Operand R = func.pushNet("R", Type(Type::TypeName::FIXED, WIDTH), flow::Net::OUT);
	Expression exprL0(L0);
	Expression exprL1(L1);

	size_t branch0 = func.pushCond(Expression::boolOf(true));
	func.conds[branch0].req(R, exprL0 || exprL1);
	//TODO: HWAT??? bitwise vs non-bitwise operators actually invert in synthesis!! (see arith::Expr tests, this if expected behavior!?)
	//func.conds[branch0].mem(m_or, exprL0 || exprL1);
	func.conds[branch0].ack({L0, L1});

	cout << parse_cog::export_func(func).to_string("") << endl;
}

TEST(FlowToFlowExport, Split) {
	using namespace flow;
	Func func;
	func.name = "split";
	Expression L = func.pushNet("L", Type(Type::TypeName::FIXED, WIDTH), flow::Net::IN);
	Expression C = func.pushNet("C", Type(Type::TypeName::FIXED, 1), flow::Net::IN);
	Expression R0 = func.pushNet("R0", Type(Type::TypeName::FIXED, WIDTH), flow::Net::OUT);
	Expression R1 = func.pushNet("R1", Type(Type::TypeName::FIXED, WIDTH), flow::Net::OUT);

	size_t branch0 = func.pushCond(C == Expression::intOf(0));
	func.conds[branch0].req(R0.top, L);
	func.conds[branch0].ack({C.top, L.top});

	size_t branch1 = func.pushCond(C == Expression::intOf(1));
	func.conds[branch1].req(R1.top, L);
	func.conds[branch1].ack({C.top, L.top});

	cout << parse_cog::export_func(func).to_string("") << endl;
}

TEST(FlowToFlowExport, Merge) {
	using namespace flow;
	Func func;
	func.name = "merge";
	Operand L0 = func.pushNet("L0", Type(Type::TypeName::FIXED, WIDTH), flow::Net::IN);
	Operand L1 = func.pushNet("L1", Type(Type::TypeName::FIXED, WIDTH), flow::Net::IN);
	Operand C = func.pushNet("C", Type(Type::TypeName::FIXED, 1), flow::Net::IN);
	Operand R = func.pushNet("R", Type(Type::TypeName::FIXED, WIDTH), flow::Net::OUT);
	Expression exprL0(L0);
	Expression exprL1(L1);
	Expression exprC(C);

	size_t branch0 = func.pushCond(exprC == Expression::intOf(0));
	func.conds[branch0].req(R, exprL0);
	func.conds[branch0].ack({C, L0});

	size_t branch1 = func.pushCond(exprC == Expression::intOf(1));
	func.conds[branch1].req(R, exprL1);
	func.conds[branch1].ack({C, L1});

	cout << parse_cog::export_func(func).to_string("") << endl;
}

TEST(FlowToFlowExport, StreamingAdder) {
	using namespace flow;
	Func func;
	func.name = "s_adder";
	Operand L = func.pushNet("L", Type(Type::TypeName::FIXED, WIDTH), flow::Net::IN);
	Operand m = func.pushNet("m", Type(Type::TypeName::FIXED, WIDTH), flow::Net::REG);
	Operand R = func.pushNet("R", Type(Type::TypeName::FIXED, WIDTH), flow::Net::OUT);
	Expression exprL(L);
	Expression exprm(m);

	size_t branch0 = func.pushCond(Expression::boolOf(true));
	func.conds[branch0].req(R, exprL + exprm);
	func.conds[branch0].mem(m, exprL);
	func.conds[branch0].ack(L);

	cout << parse_cog::export_func(func).to_string("") << endl;
}

TEST(FlowToFlowExport, SerialAdder) {
	using namespace flow;
	Func func;
	func.name = "serial_adder";
	Operand Ad = func.pushNet("Ad", Type(Type::TypeName::FIXED, WIDTH), flow::Net::IN);
	Operand Ac = func.pushNet("Ac", Type(Type::TypeName::FIXED, 1),			flow::Net::IN);
	Operand Bd = func.pushNet("Bd", Type(Type::TypeName::FIXED, WIDTH), flow::Net::IN);
	Operand Bc = func.pushNet("Bc", Type(Type::TypeName::FIXED, 1),			flow::Net::IN);
	Operand Sd = func.pushNet("Sd", Type(Type::TypeName::FIXED, WIDTH), flow::Net::OUT);
	Operand Sc = func.pushNet("Sc", Type(Type::TypeName::FIXED, 1),			flow::Net::OUT);
	Operand ci = func.pushNet("ci", Type(Type::TypeName::FIXED, 1),			flow::Net::REG);
	Expression expr_Ac(Ac);
	Expression expr_Ad(Ad);
	Expression expr_Bc(Bc);
	Expression expr_Bd(Bd);
	Expression expr_ci(ci);

	Expression expr_s((expr_Ad + expr_Bd + expr_ci) % pow(2, WIDTH));
	Expression expr_co((expr_Ad + expr_Bd + expr_ci) / pow(2, WIDTH));

	size_t branch0 = func.pushCond(!expr_Ac && !expr_Bc);
	func.conds[branch0].req(Sd, expr_s);
	func.conds[branch0].req(Sc, Expression::boolOf(false));
	func.conds[branch0].mem(ci, expr_co);
	func.conds[branch0].ack({Ac, Ad, Bc, Bd});

	size_t branch1 = func.pushCond(expr_Ac && !expr_Bc);
	func.conds[branch1].req(Sd, expr_s);
	func.conds[branch1].req(Sc, Expression::boolOf(false));
	func.conds[branch1].mem(ci, expr_co);
	func.conds[branch1].ack({Bc, Bd});

	size_t branch2 = func.pushCond(!expr_Ac && expr_Bc);
	func.conds[branch2].req(Sd, expr_s);
	func.conds[branch2].req(Sc, Expression::boolOf(false));
	func.conds[branch2].mem(ci, expr_co);
	func.conds[branch2].ack({Ac, Ad});

	size_t branch3 = func.pushCond(expr_Ac && expr_Bc && (expr_co != expr_ci));
	func.conds[branch3].req(Sd, expr_s);
	func.conds[branch3].req(Sc, Expression::boolOf(false));
	func.conds[branch3].mem(ci, expr_co);

	size_t branch4 = func.pushCond(expr_Ac && expr_Bc && (expr_co == expr_ci));
	func.conds[branch4].req(Sd, expr_s);
	func.conds[branch4].req(Sc, Expression::boolOf(true));
	func.conds[branch4].mem(ci, Expression::intOf(0));
	func.conds[branch4].ack({Ac, Ad, Bc, Bd});

	cout << parse_cog::export_func(func).to_string("") << endl;
}

TEST(FlowToFlowExport, FullAdder) {
	using namespace flow;
	using arithmetic::booleanXor;

	Func func;
	func.name = "full_adder";
	Expression A = func.pushNet("A", Type(Type::TypeName::FIXED, 1), flow::Net::IN);
	Expression B = func.pushNet("B", Type(Type::TypeName::FIXED, 1), flow::Net::IN);
	Expression S = func.pushNet("S", Type(Type::TypeName::FIXED, 1), flow::Net::OUT);
	Expression Ci = func.pushNet("Ci", Type(Type::TypeName::FIXED, 1), flow::Net::IN);
	Expression Co = func.pushNet("Co", Type(Type::TypeName::FIXED, 1), flow::Net::OUT);

	size_t branch0 = func.pushCond(Expression::boolOf(true));
	func.conds[branch0].req(S.top, booleanXor(booleanXor(A, B), Ci));
	func.conds[branch0].req(Co.top, (A && B) || (A && Ci) || (B && Ci));
	func.conds[branch0].ack({A.top, B.top, Ci.top});

	cout << parse_cog::export_func(func).to_string("") << endl;
}

