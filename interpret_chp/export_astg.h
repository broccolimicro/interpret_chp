#pragma once

#include <common/standard.h>

#include <parse_astg/graph.h>

#include <chp/graph.h>
#include <chp/state.h>

#include <parse_astg/expression.h>

#include <arithmetic/expression.h>
#include <arithmetic/state.h>
#include <arithmetic/action.h>

#include <interpret_arithmetic/export.h>

namespace parse_astg {

string export_value(const arithmetic::Value &v);

struct ExpressionExporter : arithmetic::ExpressionExporter {
	ucs::ConstNetlist nets;

	ExpressionExporter(ucs::ConstNetlist nets);
	~ExpressionExporter();

	parse_expression::operation export_operator(int func) const override;
	const parse_expression::precedence_set &precedence() const override;
	parse_expression::expression::argument export_constant(arithmetic::Value value) const override;
	parse_expression::expression::argument export_literal(size_t index) const override;
};

parse_expression::expression export_expression(const arithmetic::Expression &expr, ucs::ConstNetlist nets);
parse_expression::assignment export_assignment(const arithmetic::Action &expr, ucs::ConstNetlist nets);

struct CompositionExporter : arithmetic::CompositionExporter {
	ucs::ConstNetlist nets;

	CompositionExporter(ucs::ConstNetlist nets);
	~CompositionExporter();

	parse_expression::operation export_operator(int func) const override;
	const parse_expression::precedence_set &precedence() const override;
	parse_expression::expression::argument export_action(const arithmetic::Action &expr) const override;
};

parse_expression::expression export_composition(const arithmetic::Parallel &expr, ucs::ConstNetlist nets);
parse_expression::expression export_composition(const arithmetic::Choice &expr, ucs::ConstNetlist nets);

// ASTG

pair<parse_astg::node, parse_astg::node> export_astg(parse_astg::graph &g, parse_astg::composition c, string label);
parse_astg::graph export_astg(const chp::graph &g);

}
