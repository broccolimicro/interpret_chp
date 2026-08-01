#pragma once

#include <common/standard.h>

#include <chp/graph.h>
#include <chp/state.h>

#include <parse_cog/composition.h>
#include <parse_cog/control.h>
#include <parse_cog/declaration.h>
#include <parse_cog/expression.h>

#include <parse_expression/expression.h>
#include <parse_expression/assignment.h>
#include <parse_expression/import.h>
#include <interpret_chp/import_expr.h>

namespace parse_cog {

struct ExpressionImporter : parse_expression::Importer<arithmetic::Expression> {
	ucs::Netlist symbols;
	vector<int> region;
	bool autoDefine;

	ExpressionImporter(ucs::Netlist symbols, int region = 0, bool autoDefine = false);
	~ExpressionImporter();

	arithmetic::Expression import_term(const parse_expression::expression::argument &syntax, tokenizer *tokens) const override;
	void push_properties(parse_expression::operation op, const vector<parse_expression::expression::argument> &args, tokenizer *tokens) override;
	void pop_properties(parse_expression::operation op) override;
	arithmetic::Expression import_unary(parse_expression::operation op, arithmetic::Expression expr, tokenizer *tokens) const override;
	arithmetic::Expression import_binary(parse_expression::operation op, arithmetic::Expression left, arithmetic::Expression right, tokenizer *tokens) const override;
	arithmetic::Expression import_group(parse_expression::operation op, vector<arithmetic::Expression> args, tokenizer *tokens) const override;
	arithmetic::Expression import_modifier(parse_expression::operation op, vector<arithmetic::Expression> args, tokenizer *tokens) const override;
};

arithmetic::Expression import_expression(const parse_expression::expression &syntax, ucs::Netlist nets, tokenizer *tokens, int region = 0, bool auto_define = false);

struct CompositionImporter : parse_expression::Importer<arithmetic::Choice> {
	ucs::Netlist symbols;
	vector<int> region;
	bool autoDefine;

	CompositionImporter(ucs::Netlist symbols, int region = 0, bool autoDefine = false);
	~CompositionImporter();

	arithmetic::Action import_assignment(const assignment &syntax, tokenizer *tokens) const;
	arithmetic::Choice import_term(const parse_expression::expression::argument &syntax, tokenizer *tokens) const override;
	void push_properties(parse_expression::operation op, const vector<parse_expression::expression::argument> &args, tokenizer *tokens) override;
	void pop_properties(parse_expression::operation op) override;
	arithmetic::Choice import_binary(parse_expression::operation op, arithmetic::Choice left, arithmetic::Choice right, tokenizer *tokens) const override;
	arithmetic::Choice import_modifier(parse_expression::operation op, vector<arithmetic::Choice> args, tokenizer *tokens) const override;
};

arithmetic::Action import_assignment(const assignment &syntax, ucs::Netlist nets, tokenizer *tokens, int region = 0, bool auto_define = false);
arithmetic::Choice import_composition(const parse_expression::expression &syntax, ucs::Netlist nets, tokenizer *tokens, int region = 0, bool auto_define = false);

chp::segment import_segment(chp::graph &dst, const parse_expression::expression &syntax, string func, int default_id, tokenizer *tokens, bool auto_define);
chp::segment import_segment(chp::graph &dst, const assignment &syntax, int default_id, tokenizer *tokens, bool auto_define);

}

namespace chp {

// COG

chp::segment import_segment(chp::graph &dst, const parse_cog::composition &syntax, int default_id, tokenizer *tokens, bool auto_define);
chp::segment import_segment(chp::graph &dst, const parse_cog::control &syntax, int default_id, tokenizer *tokens, bool auto_define);
chp::segment import_segment(chp::graph &dst, const parse_cog::declaration &syntax, int default_id, tokenizer *tokens, bool auto_define);
chp::segment import_segment(chp::graph &dst, const parse::syntax *syntax, int default_id, tokenizer *tokens, bool auto_define);
void import_chp(chp::graph &dst, const parse_cog::composition &syntax, tokenizer *tokens, bool auto_define);

}
