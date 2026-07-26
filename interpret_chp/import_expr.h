#pragma once

#include <common/standard.h>

#include <chp/graph.h>
#include <chp/state.h>

#include <parse_cog/expression.h>
#include <interpret_arithmetic/import.h>

namespace chp {

struct ExpressionImporter : arithmetic::Importer<arithmetic::Expression> {
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

struct CompositionImporter : arithmetic::Importer<arithmetic::Choice> {
	ucs::Netlist symbols;
	vector<int> region;
	bool autoDefine;

	CompositionImporter(ucs::Netlist symbols, int region = 0, bool autoDefine = false);
	~CompositionImporter();

	arithmetic::Action import_assignment(const parse_cog::assignment &syntax, tokenizer *tokens) const;
	arithmetic::Choice import_term(const parse_expression::expression::argument &syntax, tokenizer *tokens) const override;
	void push_properties(parse_expression::operation op, const vector<parse_expression::expression::argument> &args, tokenizer *tokens) override;
	void pop_properties(parse_expression::operation op) override;
	arithmetic::Choice import_binary(parse_expression::operation op, arithmetic::Choice left, arithmetic::Choice right, tokenizer *tokens) const override;
	arithmetic::Choice import_modifier(parse_expression::operation op, vector<arithmetic::Choice> args, tokenizer *tokens) const override;
};

arithmetic::Action import_assignment(const parse_cog::assignment &syntax, ucs::Netlist nets, tokenizer *tokens, int region = 0, bool auto_define = false);
arithmetic::Choice import_composition(const parse_expression::expression &syntax, ucs::Netlist nets, tokenizer *tokens, int region = 0, bool auto_define = false);

struct segment {
	segment(bool cond);
	~segment();

	petri::segment nodes;
	bool loop;
	arithmetic::Expression cond;
};

segment compose(chp::graph &dst, int composition, segment s0, segment s1);

segment import_segment(chp::graph &dst, const parse_expression::expression &syntax, string func, int default_id, tokenizer *tokens, bool auto_define);
segment import_segment(chp::graph &dst, const parse_cog::assignment &syntax, int default_id, tokenizer *tokens, bool auto_define);

}
