#pragma once

#include <common/standard.h>

#include <parse_cog/composition.h>

#include <chp/graph.h>
#include <chp/state.h>
#include <petri/tree.h>

#include <parse_cog/expression.h>

#include <arithmetic/expression.h>
#include <arithmetic/state.h>
#include <arithmetic/action.h>

#include <interpret_arithmetic/export.h>

namespace parse_cog {

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



// CHP

arithmetic::Expression compute_guard(const controlflow::Tree<chp::transition, chp::place> &tree, controlflow::Index i);
bool export_control(parse_cog::composition &comp, const controlflow::Tree<chp::transition, chp::place> &tree, controlflow::Index i, controlflow::Index j, bool isEmbedded, bool isGuarded, ucs::ConstNetlist nets);
parse_cog::composition export_composition(const controlflow::Tree<chp::transition, chp::place> &tree, controlflow::Index i, bool isGuarded, ucs::ConstNetlist nets);
parse_cog::composition export_cog(const controlflow::Tree<chp::transition, chp::place> &tree, ucs::ConstNetlist nets, controlflow::Index i = controlflow::Index());

/*parse_chp::sequence export_sequence(vector<chp::iterator> &i, const chp::graph &g);
parse_chp::parallel export_parallel(vector<chp::iterator> &i, const chp::graph &g);
parse::syntax *export_condition(vector<chp::iterator> &i, const chp::graph &g);*/

/*parse_chp::parallel export_parallel(const chp::graph &g);*/



}
