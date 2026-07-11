#include "import_expr.h"

namespace chp {

segment::segment(bool cond) {
	this->loop = false;
	this->cond = cond ? arithmetic::Expression::vdd() : arithmetic::Expression::gnd();
}

segment::~segment() {
}

segment compose(chp::graph &dst, int composition, segment s0, segment s1) {
	if (composition == petri::choice) {
		s0.cond = s0.cond | s1.cond;
	} else if (composition == petri::parallel) {
		s0.cond = s0.cond & s1.cond;
	} else if (composition == petri::sequence) {
		if (s0.nodes.source.empty()) {
			s0.cond = s1.cond;
			s0.loop = s1.loop;
		}
	}
	s0.nodes = dst.compose(composition, s0.nodes, s1.nodes, true);
	return s0;
}

ExpressionInterpreter::ExpressionInterpreter(tokenizer *tokens, bool auto_define) : arithmetic::ExpressionInterpreter<parse_chp::expr_group>(tokens, auto_define) {
}

ExpressionInterpreter::ExpressionInterpreter() {
}

arithmetic::Expression ExpressionInterpreter::import_unary(operation op, arithmetic::Expression expr) {
	if (op.is("!", "", "", "")) {
		return !expr;
	} else if (op.is("~", "", "", "")) {
		return ~expr;
	} else if (op.is("+", "", "", "")) {
		return expr;
	} else if (op.is("-", "", "", "")) {
		return -expr;
	} else if (op.is("(bool)", "", "", "")) {
		return arithmetic::cast("bool", expr);
	} else if (op.is("#", "", "", "")) {
		return arithmetic::memberCall(expr, "peek", {});
	} else if (op.is("", "", "", "?")) {
		return arithmetic::memberCall(expr, "recv", {});
	}
	return expr;
}

arithmetic::Expression ExpressionInterpreter::import_binary(operation op, arithmetic::Expression left, arithmetic::Expression right) {
	if (op.is("", "", "|", "")) {
		return left | right;
	} else if (op.is("", "", "&", "")) {
		return left & right;
	} else if (op.is("", "", "^", "")) {
		return left ^ right;
	} else if (op.is("", "", "==", "")) {
		return left == right;
	} else if (op.is("", "", "!=", "")) {
		return left != right;
	} else if (op.is("", "", "<", "")) {
		return left < right;
	} else if (op.is("", "", "<=", "")) {
		return left <= right;
	} else if (op.is("", "", ">", "")) {
		return left > right;
	} else if (op.is("", "", ">=", "")) {
		return left >= right;
	} else if (op.is("", "", "||", "")) {
		return left || right;
	} else if (op.is("", "", "&&", "")) {
		return left && right;
	} else if (op.is("", "", "<<", "")) {
		return left << right;
	} else if (op.is("", "", ">>", "")) {
		return left >> right;
	} else if (op.is("", "", "+", "")) {
		return left + right;
	} else if (op.is("", "", "-", "")) {
		return left - right;
	} else if (op.is("", "", "*", "")) {
		return left * right;
	} else if (op.is("", "", "/", "")) {
		return left / right;
	} else if (op.is("", "", "%", "")) {
		return left % right;
	}
	internal("", "unrecognized operation", __FILE__, __LINE__);
	return left;
}

arithmetic::Expression ExpressionInterpreter::import_group(operation op, vector<arithmetic::Expression> args) {
	if (op.is("[", "", ",", "]")) {
		return arithmetic::array(args);
	}
	internal("", "unrecognized operation", __FILE__, __LINE__);
	return arithmetic::Expression();
}

arithmetic::Expression ExpressionInterpreter::import_modifier(operation op, const vector<argument> &arguments, ucs::Netlist nets, int default_id) {
	if (op.is("", "!", "", "")) {     // Channel Send
		vector<arithmetic::Expression> sub = import_arguments(arguments, nets, default_id);
		if (not sub.empty()) {
			return arithmetic::memberCall("send", sub);
		} else {
			error("", "operator '!' expects at least one operand", __FILE__, __LINE__);
			return arithmetic::memberCall("send", {arithmetic::Expression::undef()});
		}
	// TODO(edward.bingham) This operator is specific to QDI languages (CHP, HSE, PRS, COG)
	} else if (op.is("", "'", "", "")) { // Region
		if (arguments.size() != 2u) {
			error("", "operator ''' expects 2 arguments, found " + ::to_string(arguments.size()), __FILE__, __LINE__);
		}
		string cnst = import_constant(arguments[1]);
		return import_argument(arguments[0], nets, atoi(cnst.c_str()));
	} else if (op.is("", ".", "", "")) { // Member
		return import_members(arguments, nets, default_id);
	// DESIGN(edward.bingham) Move "this" into the first argument of the
	// function. So "a.b.c(d, e) becomes c(a.b, d, e). This seems like a
	// reasonable way to simplify things, and follows the early style of c++
	// function names.
	} else if (op.is("", "(", ",", ")")) { // Call, Validity, Truthiness
		vector<arithmetic::Expression> sub = import_call(arguments, nets, default_id);
		if (sub.empty()) {
			error("", "function call expects function name", __FILE__, __LINE__);
			return arithmetic::Expression();
		}

		// Replace member calls
		if (sub[0].top.isExpr() and sub[0].getExpr(sub[0].top.index)->func == arithmetic::Operation::MEMBER) {
			arithmetic::Operation op = *sub[0].getExpr(sub[0].top.index);

			arithmetic::Operand name = op.operands.back();
			op.operands.pop_back();

			if (op.operands.size() == 1u) {
				op.func = arithmetic::Operation::IDENTITY;
			}
			sub[0].setExpr(op);
			sub.insert(sub.begin(), name);

			return arithmetic::Expression(arithmetic::Operation::MEMBER_CALL, sub);
		// Replace built-in functions
		} else if (sub[0].top.isConst() and sub[0].top.cnst.type == arithmetic::Value::STRING and sub[0].top.cnst.sval == "valid") {
			if      (sub.size() == 1u) { return arithmetic::Expression::vdd(); }
			else if (sub.size() == 2u) { return arithmetic::isValid(sub[1]); }
			else { error("", "valid() function expects 1 argument, found " + ::to_string(sub.size()-1), __FILE__, __LINE__); }
		} else if (sub[0].top.isConst() and sub[0].top.cnst.type == arithmetic::Value::STRING and sub[0].top.cnst.sval == "true") {
			if      (sub.size() == 1u) { return arithmetic::Expression::boolOf(true); }
			else if (sub.size() == 2u) { return arithmetic::isTrue(sub[1]); }
			else { error("", "true() function expects 1 argument, found " + ::to_string(sub.size()-1), __FILE__, __LINE__); }
		} else {
			return arithmetic::Expression(arithmetic::Operation::CALL, sub);
		}
	// END DESIGN
	} else if (op.is("", "[", ":", "]")) {
		return arithmetic::Expression(arithmetic::Operation::INDEX, import_arguments(arguments, nets, default_id));
	}
	internal("", "unrecognized operation", __FILE__, __LINE__);
	return arithmetic::Expression();
}

arithmetic::Expression import_expression(const parse_chp::expression &syntax, ucs::Netlist nets, int default_id, tokenizer *tokens, bool auto_define) {
	ExpressionInterpreter intr(tokens, auto_define);
	return intr.import_expression(syntax, nets, default_id);
}

arithmetic::State import_state(const parse_chp::simple_composition &syntax, ucs::Netlist nets, int default_id, tokenizer *tokens, bool auto_define) {
	ExpressionInterpreter intr(tokens, auto_define);
	return intr.import_state(syntax, nets, default_id);
}

}
