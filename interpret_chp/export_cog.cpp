#include "export_cog.h"

#include <arithmetic/algorithm.h>
#include <common/message.h>

#include <parse/wrapper.h>

namespace parse_cog {

ExpressionExporter::ExpressionExporter(ucs::ConstNetlist nets) : nets(nets) {
}

ExpressionExporter::~ExpressionExporter() {
}

parse_expression::operation ExpressionExporter::export_operator(int func) const {
	using OpType = arithmetic::Operation::OpType;
	using operation = parse_expression::operation;

	switch (func) {
	// VALIDITY - converted to CALL
	case OpType::WIRE_NOT: return operation("~", "", "", "");
	case OpType::WIRE_OR:  return operation("", "", "|", "");
	case OpType::WIRE_AND: return operation("", "", "&", "");
	case OpType::WIRE_XOR: return operation("", "", "^", "");
	// TRUTHINESS - converted to CALL
	case OpType::BOOLEAN_NOT: return operation("!", "", "", "");
	case OpType::BOOLEAN_OR: return operation("", "", "||", "");
	case OpType::BOOLEAN_AND: return operation("", "", "&&", "");
	case OpType::BOOLEAN_XOR: return operation("", "", "^^", "");
	case OpType::EQUAL: return operation("", "", "==", "");
	case OpType::NOT_EQUAL: return operation("", "", "!=", "");
	case OpType::LESS: return operation("", "", "<", "");
	case OpType::GREATER: return operation("", "", ">", "");
	case OpType::LESS_EQUAL: return operation("", "", "<=", "");
	case OpType::GREATER_EQUAL: return operation("", "", ">=", "");
	// NEGATIVE - converted to LESS
	case OpType::TERNARY: return operation("", "?", ":", "");
	case OpType::IDENTITY: return operation("+", "", "", "");
	case OpType::NEGATION: return operation("-", "", "", "");
	// INVERSE - converted to DIVIDE
	// TODO(edward.bingham) we need type information here to determine if we are using arithmetic or logical shift
	case OpType::SHIFT_LEFT: return operation("", "", "<<", "");
	case OpType::SHIFT_RIGHT: return operation("", "", ">>", "");
	case OpType::ADD: return operation("", "", "+", "");
	case OpType::SUBTRACT: return operation("", "", "-", "");
	case OpType::MULTIPLY: return operation("", "", "*", "");
	case OpType::DIVIDE: return operation("", "", "/", "");
	case OpType::MOD: return operation("", "", "%", "");
	case OpType::CALL: return operation("", "(", ",", ")");
	// MEMBER_CALL - converted to MEMBER and CALL
	//case OpType::CAST: return operation("", "(", "", ")");
	case OpType::ARRAY: return operation("[", "", ",", "]");
	case OpType::INDEX: return operation("", "[", ":", "]");
	//case OpType::STRUCT: return operation("'{", "", "", "}");
	case OpType::MEMBER: return operation("", ".", "", "");
	}
	return operation();
}

const parse_expression::precedence_set &ExpressionExporter::precedence() const {
	return expression_config::cfg->order;
}

parse_expression::expression::argument ExpressionExporter::export_constant(arithmetic::Value value) const {
	if (value.type == arithmetic::Value::TYPE) {
		type_name result;
		result.value = arithmetic::export_value(value);
		return {2, std::shared_ptr<parse::syntax>(result.clone())};
	} else if (value.type == arithmetic::Value::TERM) {
		term_name result;
		result.value = arithmetic::export_value(value);
		return {3, std::shared_ptr<parse::syntax>(result.clone())};
	}
	constant result;
	result.value = arithmetic::export_value(value);
	return {0, std::shared_ptr<parse::syntax>(result.clone())};
}

parse_expression::expression::argument ExpressionExporter::export_literal(size_t index) const {
	literal result;
	result.name = nets.netAt(index);
	return {1, std::shared_ptr<parse::syntax>(result.clone())};
}

parse_expression::expression export_expression(const arithmetic::Expression &expr, ucs::ConstNetlist nets) {
	return ExpressionExporter(nets).export_expression(expr);
}

parse_expression::assignment export_assignment(const arithmetic::Action &expr, ucs::ConstNetlist nets) {
	parse_expression::assignment result;
	result.valid = true;

	if (not expr.lvalue.isUndef()) {
		result.left.push_back(export_expression(expr.lvalue, nets));
	}

	// TODO(edward.bingham) we need type information about the lvalue here
	arithmetic::Operand top = expr.rvalue.top;
	if (top.isConst() and top.cnst.isNeutral()) {
		result.operation = "-";
	} else if (top.isConst() and top.cnst.isUnstable()) {
		result.operation = "~";
	} else if (top.isConst() and top.cnst.type == arithmetic::Value::WIRE and top.cnst.isValid()) {
		result.operation = "+";
	} else {
		result.right = export_expression(expr.rvalue, nets);
		result.operation = "=";
	}

	return result;
}

CompositionExporter::CompositionExporter(ucs::ConstNetlist nets) : nets(nets) {
}

CompositionExporter::~CompositionExporter() {
}

parse_expression::operation CompositionExporter::export_operator(int func) const {
	using OpType = arithmetic::Operation::OpType;
	using operation = parse_expression::operation;

	switch (func) {
	// VALIDITY - converted to CALL
	case OpType::WIRE_OR:  return operation("", "", ":", "");
	case OpType::WIRE_AND: return operation("", "", ",", "");
	}
	return operation();
}

const parse_expression::precedence_set &CompositionExporter::precedence() const {
	return composition_config::cfg->order;
}

parse_expression::expression::argument CompositionExporter::export_action(const arithmetic::Action &expr) const {
	return {1, std::shared_ptr<parse::syntax>(export_assignment(expr, nets).clone())};
}

parse_expression::expression export_composition(const arithmetic::Parallel &expr, ucs::ConstNetlist nets) {
	return CompositionExporter(nets).export_expression(expr);
}

parse_expression::expression export_composition(const arithmetic::Choice &expr, ucs::ConstNetlist nets) {
	return CompositionExporter(nets).export_expression(expr);
}

}
