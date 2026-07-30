#include "import_chp.h"
#include "import_expr.h"

#include <common/standard.h>
#include <parse_expression/import.h>
#include <interpret_arithmetic/import_default.h>

namespace parse_chp {

ExpressionImporter::ExpressionImporter(ucs::Netlist symbols, int region, bool autoDefine) : symbols(symbols) {
	this->region.push_back(region);
	this->autoDefine = autoDefine;
}

ExpressionImporter::~ExpressionImporter() {
}

arithmetic::Expression ExpressionImporter::import_term(const parse_expression::expression::argument &syntax, tokenizer *tokens) const {
	if (syntax.type < 0 or syntax.type >= (int)expression_config::cfg->literals.size() or not syntax.ptr) {
		return arithmetic::Expression::undef();
	}

	std::string type = expression_config::cfg->literals[syntax.type].first;

	if (type == "constant") {
		std::string value = syntax.ptr->get<constant>().value;
		return arithmetic::import_constant(value, tokens);
	} else if (type == "literal") {
		std::string name = syntax.ptr->get<literal>().name;
		if (region.back() != 0) {
			name += "'" + std::to_string(region.back());
		}
		return arithmetic::import_literal(name, symbols, tokens, autoDefine);
	} else if (type == "type") {
		std::string name = syntax.ptr->get<type_name>().value;
		return arithmetic::Expression::typeOf(name);
	} else if (type == "term") {
		std::string name = syntax.ptr->get<term_name>().value;
		return arithmetic::Expression::termOf(name);
	} else if (type == "label") {
		std::string value = syntax.ptr->get<label>().value;
		return arithmetic::import_constant(value, tokens);
	}
	internal("", "unsupported literal type '" + type + "'", __FILE__, __LINE__);
	return arithmetic::Expression::undef();
}

void ExpressionImporter::push_properties(parse_expression::operation op, const vector<parse_expression::expression::argument> &args, tokenizer *tokens) {
	if (op.is("", "'", "", "")) { // Region
		int value = -1;
		if (args.size() == 2u) {
			std::string str = args[1].ptr->to_string("");
			value = atoi(str.c_str());
		} else {
			error("", "operator ''' expects 2 arguments, found '" + ::to_string(args.size()) + "'", __FILE__, __LINE__);
		}
		this->region.push_back(value);
	}
}

void ExpressionImporter::pop_properties(parse_expression::operation op) {
	if (op.is("", "'", "", "")) { // Region
		region.pop_back();
	}
}

arithmetic::Expression ExpressionImporter::import_unary(parse_expression::operation op, arithmetic::Expression expr, tokenizer *tokens) const {
	if (op.is("!", "", "", "")) {
		return !expr;
	} else if (op.is("~", "", "", "")) {
		return ~expr;
	} else if (op.is("+", "", "", "")) {
		return expr;
	} else if (op.is("-", "", "", "")) {
		return -expr;
	} else if (op.is("#", "", "", "")) {
		return arithmetic::memberCall("peek", {expr});
	} else if (op.is("", "", "", "?")) {
		return arithmetic::memberCall("recv", {expr});
	}
	return expr;
}

arithmetic::Expression ExpressionImporter::import_binary(parse_expression::operation op, arithmetic::Expression left, arithmetic::Expression right, tokenizer *tokens) const {
	if (op.is("", "", "|", "")) {
		return left | right;
	} else if (op.is("", "", "&", "")) {
		return left & right;
	} else if (op.is("", "", "^", "")) {
		return left ^ right;
	} else if (op.is("", "", "||", "")) {
		return left || right;
	} else if (op.is("", "", "&&", "")) {
		return left && right;
	} else if (op.is("", "", "^^", "")) {
		return booleanXor(left, right);
	} else if (op.is("", "", "==", "")) {
		return left == right;
	} else if (op.is("", "", "!=", "")) {
		return left != right;
	} else if (op.is("", "", "<", "")) {
		return left < right;
	} else if (op.is("", "", ">", "")) {
		return left > right;
	} else if (op.is("", "", "<=", "")) {
		return left <= right;
	} else if (op.is("", "", ">=", "")) {
		return left >= right;
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

/*
TODO(edward.bingham) we don't have arithmetic support for conditionals inside expressions

arithmetic::Expression ExpressionImporter::import_ternary(parse_expression::operation op, std::vector<arithmetic::Expression> args, tokenizer *tokens) const {
	if (op.is("", "?", ":", "")) {
		return left | right;
	}
	internal("", "unrecognized operation", __FILE__, __LINE__);
	return left;
}*/


arithmetic::Expression ExpressionImporter::import_group(parse_expression::operation op, vector<arithmetic::Expression> args, tokenizer *tokens) const {
	if (op.is("[", "", ",", "]")) {
		return arithmetic::array(args);
	}
	internal("", "unrecognized operation", __FILE__, __LINE__);
	return arithmetic::Expression();
}

arithmetic::Expression ExpressionImporter::import_modifier(parse_expression::operation op, vector<arithmetic::Expression> args, tokenizer *tokens) const {
	// TODO(edward.bingham) See the parser, operator :: is not yet implemented

	if (op.is("", "!", "", "")) {     // Channel Send
		if (not args.empty()) {
			return arithmetic::memberCall("send", args);
		} else {
			error("", "operator '!' expects at least one operand", __FILE__, __LINE__);
			return arithmetic::memberCall("send", {arithmetic::Expression::undef()});
		}
	} else if (op.is("", "'", "", "")) { // Region
		// only affects properties
		return args[0];
	} else if (op.is("", ".", "", "")) { // Member
		return arithmetic::Expression(arithmetic::Operation::MEMBER, args);
	// DESIGN(edward.bingham) Move "this" into the first argument of the
	// function. So "a.b.c(d, e) becomes c(a.b, d, e). This seems like a
	// reasonable way to simplify things, and follows the early style of c++
	// function names.
	} else if (op.is("", "(", ",", ")")) { // Call, Validity, Truthiness
		if (args.empty()) {
			error("", "function call expects function name", __FILE__, __LINE__);
			return arithmetic::Expression();
		}

		// Replace member calls
		if (args[0].top.isExpr() and args[0].getExpr(args[0].top.index)->func == arithmetic::Operation::MEMBER) {
			arithmetic::Operation op = *args[0].getExpr(args[0].top.index);

			arithmetic::Operand name = op.operands.back();
			op.operands.pop_back();

			if (op.operands.size() == 1u) {
				op.func = arithmetic::Operation::IDENTITY;
			}
			args[0].setExpr(op);
			args.insert(args.begin(), name);

			return arithmetic::Expression(arithmetic::Operation::MEMBER_CALL, args);
		// Replace built-in functions
		} else if (args[0].top.isConst() and args[0].top.cnst.type == arithmetic::Value::STRING and args[0].top.cnst.sval == "valid") {
			if      (args.size() == 1u) { return arithmetic::Expression::vdd(); }
			else if (args.size() == 2u) { return arithmetic::isValid(args[1]); }
			else { error("", "valid() function expects 1 argument, found " + ::to_string(args.size()-1), __FILE__, __LINE__); }
		} else if (args[0].top.isConst() and args[0].top.cnst.type == arithmetic::Value::STRING and args[0].top.cnst.sval == "true") {
			if      (args.size() == 1u) { return arithmetic::Expression::boolOf(true); }
			else if (args.size() == 2u) { return arithmetic::isTrue(args[1]); }
			else { error("", "true() function expects 1 argument, found " + ::to_string(args.size()-1), __FILE__, __LINE__); }
		} else {
			return arithmetic::Expression(arithmetic::Operation::CALL, args);
		}
	// END DESIGN
	} else if (op.is("", "[", ":", "]")) {
		return arithmetic::Expression(arithmetic::Operation::INDEX, args);
	}
	internal("", "unrecognized operation", __FILE__, __LINE__);
	return arithmetic::Expression();
}

arithmetic::Expression import_expression(const parse_expression::expression &syntax, ucs::Netlist nets, tokenizer *tokens, int region, bool auto_define) {
	return ExpressionImporter(nets, region, auto_define).import_expression(syntax, tokens);
}

CompositionImporter::CompositionImporter(ucs::Netlist symbols, int region, bool autoDefine) : symbols(symbols) {
	this->region.push_back(region);
	this->autoDefine = autoDefine;
}

CompositionImporter::~CompositionImporter() {
}

arithmetic::Action CompositionImporter::import_assignment(const assignment &syntax, tokenizer *tokens) const {
	ExpressionImporter in(symbols, region.back(), autoDefine);

	arithmetic::Action result;
	if (syntax.operation.empty()) {
		result.lvalue = arithmetic::Expression::undef();
		if (syntax.left[0].valid) {
			result.rvalue = in.import_expression(syntax.left[0], tokens);
		}
	} else if (syntax.operation == "+") {
		if (syntax.left.size() > 0) {
			result.lvalue = in.import_expression(syntax.left[0], tokens);
		}
		result.rvalue = arithmetic::Expression::vdd();
	} else if (syntax.operation == "-") {
		if (syntax.left.size() > 0) {
			result.lvalue = in.import_expression(syntax.left[0], tokens);
		}
		result.rvalue = arithmetic::Expression::gnd();
	} else if (syntax.operation == "=") {
		if (syntax.left.size() > 0) {
			result.lvalue = in.import_expression(syntax.left[0], tokens);
		}
		if (syntax.right.valid) {
			result.rvalue = in.import_expression(syntax.right, tokens);
		}
	}
	return result;
}

arithmetic::Choice CompositionImporter::import_term(const parse_expression::expression::argument &syntax, tokenizer *tokens) const {
	if (syntax.type < 0 or syntax.type >= (int)expression_config::cfg->literals.size() or not syntax.ptr) {
		return arithmetic::Choice();
	}

	return arithmetic::Choice({{import_assignment(syntax.ptr->get<assignment>(), tokens)}});
}

void CompositionImporter::push_properties(parse_expression::operation op, const vector<parse_expression::expression::argument> &args, tokenizer *tokens) {
	if (op.is("", "'", "", "")) { // Region
		int value = -1;
		if (args.size() == 2u) {
			std::string str = args[1].ptr->to_string("");
			value = atoi(str.c_str());
		} else {
			error("", "operator ''' expects 2 arguments, found '" + ::to_string(args.size()) + "'", __FILE__, __LINE__);
		}
		this->region.push_back(value);
	}
}

void CompositionImporter::pop_properties(parse_expression::operation op) {
	if (op.is("", "'", "", "")) { // Region
		region.pop_back();
	}
}

arithmetic::Choice CompositionImporter::import_modifier(parse_expression::operation op, vector<arithmetic::Choice> args, tokenizer *tokens) const {
	if (op.is("", "'", "", "")) {
		return args[0];
	}
	return parse_expression::Importer<arithmetic::Choice>::import_modifier(op, args, tokens);
}

arithmetic::Choice CompositionImporter::import_binary(parse_expression::operation op, arithmetic::Choice left, arithmetic::Choice right, tokenizer *tokens) const {
	if (op.is("", "", ":", "")) {
		return left | right;
	} else if (op.is("", "", ",", "")) {
		return left & right;
	}
	internal("", "unrecognized operation", __FILE__, __LINE__);
	return left;
}

arithmetic::Action import_assignment(const assignment &syntax, ucs::Netlist nets, tokenizer *tokens, int region, bool auto_define) {
	return CompositionImporter(nets, region, auto_define).import_assignment(syntax, tokens);
}

arithmetic::Choice import_composition(const parse_expression::expression &syntax, ucs::Netlist nets, tokenizer *tokens, int region, bool auto_define) {
	return CompositionImporter(nets, region, auto_define).import_expression(syntax, tokens);
}

chp::segment import_segment(chp::graph &dst, const parse_expression::expression &syntax, string func, int default_id, tokenizer *tokens, bool auto_define) {
	chp::segment result(false);
	result.cond = import_expression(syntax, dst, tokens, default_id, auto_define);
	if (func == "await") {
		result.cond = arithmetic::isValid(result.cond);
	} else if (func == "if") {
		result.cond = arithmetic::isTrue(result.cond);
	}
	petri::iterator t = dst.create(chp::transition(result.cond));
	result.nodes = petri::segment({{t}}, {{t}});
	return result;
}

chp::segment import_segment(chp::graph &dst, const assignment &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	static const auto Vdd = arithmetic::Operand::vdd();

	chp::segment result(true);
	petri::iterator t = dst.create(chp::transition(Vdd,
		{{import_assignment(syntax, dst, tokens, default_id, auto_define)}}));
	result.nodes = petri::segment({{t}}, {{t}});
	return result;
}


petri::segment import_segment(chp::graph &dst, const parse_chp::composition &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	if (syntax.region != "") {
		default_id = atoi(syntax.region.c_str());
	}

	petri::segment result;

	int composition = petri::parallel;
	if (parse_chp::composition::precedence[syntax.level] == "||" or parse_chp::composition::precedence[syntax.level] == ",") {
		composition = petri::parallel;
	} else if (parse_chp::composition::precedence[syntax.level] == ";") {
		composition = petri::sequence;
	}

	for (int i = 0; i < (int)syntax.branches.size(); i++) {
		petri::segment branch;
		if (syntax.branches[i].sub.valid) {
			branch = import_segment(dst, syntax.branches[i].sub, default_id, tokens, auto_define);
		} else if (syntax.branches[i].ctrl.valid) {
			branch = import_segment(dst, syntax.branches[i].ctrl, default_id, tokens, auto_define);
		} else if (syntax.branches[i].assign.valid) {
			branch = import_segment(dst, syntax.branches[i].assign, default_id, tokens, auto_define).nodes;
		} else {
			continue;
		}

		result = dst.compose(composition, result, branch);

		if (syntax.reset == 0 and i == 0) {
			result.reset = result.source;
		} else if (syntax.reset == i+1) {
			result.reset = result.sink;
		}
	}

	if (syntax.branches.size() == 0) {
		petri::iterator b = dst.create(chp::place());

		result.source = petri::bound({{b}});
		result.sink = petri::bound({{b}});

		if (syntax.reset >= 0) {
			result.reset = result.source;
		}
	}

	return result;
}

petri::segment import_segment(chp::graph &dst, const parse_chp::control &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	if (syntax.region != "") {
		default_id = atoi(syntax.region.c_str());
	}

	petri::segment result;
	if (syntax.branches.empty()) {
		return result;
	}

	for (int i = 0; i < (int)syntax.branches.size(); i++) {
		petri::segment branch;
		if (syntax.branches[i].first.valid and not import_expression(syntax.branches[i].first, dst, tokens, default_id, auto_define).isValid()) {
			branch = dst.compose(petri::sequence, branch, import_segment(dst, syntax.branches[i].first, "await", default_id, tokens, auto_define).nodes);
		}
		if (syntax.branches[i].second.valid) {
			branch = dst.compose(petri::sequence, branch, import_segment(dst, syntax.branches[i].second, default_id, tokens, auto_define));
		}

		result = dst.compose(petri::choice, result, branch);
	}

	if (result.source.size() > 1u) {
		petri::iterator p = dst.create(chp::place());
		dst.connect({{p}}, result.source);
		result.source = petri::bound({{p}});
	}

	if (result.sink.size() > 1u) {
		petri::iterator p = dst.create(chp::place());
		dst.connect(result.sink, {{p}});
		result.sink = petri::bound({{p}});
	}

	if (not syntax.deterministic) {
		for (auto i = result.source.begin(); i != result.source.end(); i++) {
			for (auto j = i->begin(); j != i->end(); j++) {
				dst.places[j->index].arbiter = true;
			}
		}
	}

	if (syntax.repeat) {
		dst.connect(result.sink, result.source);
		result.sink.clear();

		arithmetic::Expression repeat(true);
		for (int i = 0; i < (int)syntax.branches.size(); i++) {
			if (syntax.branches[i].first.valid) {
				if (i == 0) {
					repeat = ~import_expression(syntax.branches[i].first, dst, tokens, default_id, auto_define);
				} else {
					repeat = repeat & !import_expression(syntax.branches[i].first, dst, tokens, default_id, auto_define);
				}
			} else {
				repeat = arithmetic::Expression::boolOf(false);
				break;
			}
		}

		if (!repeat.isNeutral()) {
			chp::iterator guard = dst.create(chp::transition(repeat, arithmetic::Choice(true)));
			dst.connect(result.source, petri::bound({{guard}}));
			chp::iterator arrow = dst.create(chp::place());
			dst.connect(guard, arrow);

			result.sink = petri::bound({{arrow}});
		}

		if (not result.reset.empty()) {
			result.source = result.reset;
			result.reset.clear();
		}
	}

	return result;
}

void import_chp(chp::graph &dst, const parse_chp::composition &syntax, tokenizer *tokens, bool auto_define) {
	petri::segment result = import_segment(dst, syntax, 0, tokens, auto_define);
	if (not result.reset.empty()) {
		result.source = result.reset;
		result.reset.clear();
	}

	if (result.source.size() > 1u) {
		petri::iterator p = dst.create(chp::place());
		dst.connect({{p}}, result.source);
		result.source = petri::bound({{p}});
	} else {
		for (auto i = result.source.begin(); i != result.source.end(); i++) {
			for (int j = (int)i->size()-1; j >= 0; j--) {
				if (i->nodes[j].type == chp::transition::type) {
					vector<petri::iterator> p = dst.prev(i->nodes[j]);
					if (p.empty()) {
						p.push_back(dst.create(chp::place()));
						dst.connect(p.back(), i->nodes[j]);
					}
					i->nodes.erase(i->nodes.begin()+j);
					i->nodes.insert(i->nodes.end(), p.begin(), p.end());
				}
			}
		}
	}

	vector<chp::state> reset;
	for (auto i = result.source.begin(); i != result.source.end(); i++) {
		chp::state rst;
		for (auto j = i->begin(); j != i->end(); j++) {
			rst.tokens.push_back(petri::token(j->index));
		}
		reset.push_back(rst);
	}

	if (dst.reset.empty()) {
		dst.reset = reset;
	} else {
		vector<chp::state> prev = dst.reset;
		dst.reset.clear();

		for (auto i = prev.begin(); i != prev.end(); i++) {
			for (auto j = reset.begin(); j != reset.end(); j++) {
				dst.reset.push_back(chp::state::merge(*i, *j));
			}
		}
	}
}

}
