#include "import_expr.h"
#include "import_astg.h"
#include <parse_expression/import.h>
#include <interpret_arithmetic/import_default.h>

namespace parse_astg {

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
	} else if (type == "label") {
		std::string name = syntax.ptr->get<label>().value;
		return arithmetic::Expression::labelOf(name);
	} else if (type == "ident") {
		std::string value = syntax.ptr->get<ident>().value;
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

	/*if (op.is("", "!", "", "")) {     // Channel Send
		if (not args.empty()) {
			return arithmetic::memberCall("send", args);
		} else {
			error(__FILE__, __LINE__, nullptr, nullptr, "operator '!' expects at least one operand");
			return arithmetic::memberCall("send", {arithmetic::Expression::undef()});
		}
	// TODO(edward.bingham) This operator is specific to QDI languages (CHP, HSE, PRS, COG)
	} else*/ if (op.is("", "'", "", "")) { // Region
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

}

namespace chp {

chp::iterator import_chp(const parse_astg::node &syntax, chp::graph &g, map<string, chp::iterator> &ids, tokenizer *tokens)
{
	chp::iterator i(-1,-1);
	if (syntax.id.size() > 0) {
		i.type = chp::transition::type;
		i.index = std::stoi(syntax.id);
	} else if (syntax.place.size() > 0 && syntax.place[0] == 'p') {
		i.type = chp::place::type;
		i.index = std::stoi(syntax.place.substr(1));
	} else if (tokens != NULL) {
		tokens->load(&syntax);
		tokens->error("Undefined node", __FILE__, __LINE__);
		return i;
	} else {
		error("", "Undefined node \"" + syntax.to_string() + "\"", __FILE__, __LINE__);
		return i;
	}
	
	auto created = ids.insert(pair<string, chp::iterator>(syntax.to_string(), i));
	if (created.second && i.type == chp::transition::type)
	{
		arithmetic::Expression guard(true);
		arithmetic::Choice action;
		if (syntax.guard.valid) {
			guard = import_expression(syntax.guard, g, tokens, 0, false);
		}
		if (syntax.assign.valid) {
			action = import_composition(syntax.assign, g, tokens, 0, false);
		} else {
			action.terms.push_back(arithmetic::Parallel());
		}

		g.create_at(chp::transition(guard, action), i.index);
	} else if (created.second) {
		g.create_at(chp::place(), i.index);
	}

	return i;
}

void import_chp(const parse_astg::arc &syntax, chp::graph &g, map<string, chp::iterator> &ids, tokenizer *tokens)
{
	chp::iterator base = import_chp(syntax.nodes[0], g, ids, tokens);
	for (int i = 1; i < (int)syntax.nodes.size(); i++)
	{
		chp::iterator next = import_chp(syntax.nodes[i], g, ids, tokens);
		g.connect(base, next);
	}
}

chp::graph import_chp(const parse_astg::graph &syntax, tokenizer *tokens)
{
	chp::graph result;
	map<string, chp::iterator> ids;
	for (int i = 0; i < (int)syntax.inputs.size(); i++)
		arithmetic::import_literal(syntax.inputs[i].to_string(), result, tokens, true);

	for (int i = 0; i < (int)syntax.outputs.size(); i++)
		arithmetic::import_literal(syntax.outputs[i].to_string(), result, tokens, true);

	for (int i = 0; i < (int)syntax.internal.size(); i++)
		arithmetic::import_literal(syntax.internal[i].to_string(), result, tokens, true);

	for (int i = 0; i < (int)syntax.arcs.size(); i++)
		import_chp(syntax.arcs[i], result, ids, tokens);

	for (int i = 0; i < (int)syntax.marking.size(); i++) {
		chp::state rst;
		if (syntax.marking[i].first.valid) {
			arithmetic::Region region = import_composition(syntax.marking[i].first, result, tokens, 0, false).evaluate(arithmetic::State());
			if (region.states.size() != 1u) {
				error("", "expected exactly one reset state", __FILE__, __LINE__);
			}
			if (not region.states.empty()) {
				rst.encodings = region.states[0];
			}
		}

		for (int j = 0; j < (int)syntax.marking[i].second.size(); j++) {
			chp::iterator loc = import_chp(syntax.marking[i].second[j], result, ids, tokens);
			if (loc.type == chp::place::type && loc.index >= 0)
				rst.tokens.push_back(loc.index);
		}
		result.reset.push_back(rst);
	}

	for (int i = 0; i < (int)syntax.arbiter.size(); i++) {
		chp::iterator loc = import_chp(syntax.arbiter[i], result, ids, tokens);
		if (loc.type == chp::place::type and loc.index >= 0) {
			result.places[loc.index].arbiter = true;
		}
	}

	return result;
}

}
