#include "export_cog.h"

#include <parse_cog/control.h>

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
	// INVERSE - converted to INTDIV
	// TODO(edward.bingham) we need type information here to determine if we are using arithmetic or logical shift
	case OpType::SHIFT_LEFT: return operation("", "", "<<", "");
	case OpType::SHIFT_RIGHT: return operation("", "", ">>", "");
	case OpType::ADD: return operation("", "", "+", "");
	case OpType::SUBTRACT: return operation("", "", "-", "");
	case OpType::MULTIPLY: return operation("", "", "*", "");
	case OpType::INTDIV: return operation("", "", "/", "");
	case OpType::INTMOD: return operation("", "", "%", "");
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
	if (value.type == arithmetic::Value::LABEL) {
		label result;
		result.value = arithmetic::export_value(value);
		return {2, std::shared_ptr<parse::syntax>(result.clone())};
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


arithmetic::Expression compute_guard(const controlflow::Tree<chp::transition, chp::place> &tree, controlflow::Index i) {
	if (i.type == controlflow::Index::TRANSITION) {
		return tree.transitions[i.index].guard;
			//& isValid(tree.transitions[i.index].action.guard());
	}

	if (tree.nodes[i.index].type == controlflow::Node::SEQUENCE) {
		if (tree.nodes[i.index].procs.empty()) {
			return arithmetic::Expression::vdd();
		}
		return compute_guard(tree, tree.nodes[i.index].procs[0]);
	} else if (tree.nodes[i.index].type == controlflow::Node::PARALLEL) {
		arithmetic::Expression guard = arithmetic::Expression::vdd();
		for (auto proc : tree.nodes[i.index].procs) {
			guard = guard & compute_guard(tree, proc);
		}
		return guard;
	}

	arithmetic::Expression guard = arithmetic::Expression::gnd();
	for (auto proc : tree.nodes[i.index].procs) {
		guard = guard | compute_guard(tree, proc);
	}
	return guard;
}

arithmetic::Expression popValid(arithmetic::Expression expr) {
	if (expr.top.isExpr()) {
		const arithmetic::Operation *op = expr.getExpr(expr.top.index);
		if (op->func == arithmetic::Operation::VALIDITY and not op->operands.empty()) {
			expr.top = expr.getExpr(expr.top.index)->operands.back();
		}
	}
	return expr;
}

bool hasTrue(arithmetic::Expression expr) {
	if (expr.top.isExpr()) {
		const arithmetic::Operation *op = expr.getExpr(expr.top.index);
		if (op->func == arithmetic::Operation::TRUTHINESS and not op->operands.empty()) {
			return true;
		}
	}
	return false;
}

arithmetic::Expression popTrue(arithmetic::Expression expr) {
	if (expr.top.isExpr()) {
		const arithmetic::Operation *op = expr.getExpr(expr.top.index);
		if (op->func == arithmetic::Operation::TRUTHINESS and not op->operands.empty()) {
			expr.top = expr.getExpr(expr.top.index)->operands.back();
		}
	}
	return expr;
}

bool export_control(parse_cog::composition &comp, const controlflow::Tree<chp::transition, chp::place> &tree, controlflow::Index i, controlflow::Index j, bool isEmbedded, bool isGuarded, ucs::ConstNetlist nets) {
	if (i.type == controlflow::Index::TRANSITION) {
		const chp::transition &t = tree.transitions[i.index];
		if (not t.guard.isUndef() and not t.guard.isValid() and not isGuarded) {
			parse_cog::control result;
			result.valid = true;
			result.kind = "await";
			arithmetic::Expression guard = popValid(t.guard);
			if (hasTrue(guard)) {
				result.kind = "if";
				guard = popTrue(guard);
			}
			result.guard = export_expression(guard, nets);
			if (isEmbedded and not t.action.isInfeasible() and not t.action.isVacuous()) {
				result.action.valid = true;
				result.action.level = parse_cog::composition::SEQUENCE;
				result.action.branches.push_back(std::shared_ptr<parse::syntax>(export_composition(t.action, nets).clone()));
			}
			comp.branches.push_back(std::shared_ptr<parse::syntax>(result.clone()));
		}
		if ((t.guard.isUndef() or t.guard.isValid()
			or isGuarded or not isEmbedded)
			and not t.action.isInfeasible()
			and not t.action.isVacuous()) {
			comp.branches.push_back(std::shared_ptr<parse::syntax>(export_composition(t.action, nets).clone()));
		}
		return false;
	}

	
	if (tree.nodes[i.index].type == controlflow::Node::LOOP) {
		parse_cog::control result;
		result.valid = true;
		result.action = export_composition(tree, i, false, nets);
		result.kind = "xuntil";
		if (tree.nodes[i.index].place >= 0) {
			result.kind = tree.places[tree.nodes[i.index].place].arbiter ? "xuntil" : "until";
		}
		arithmetic::Expression guard = j.index >= 0 ? compute_guard(tree, j) : arithmetic::Expression::gnd();
		if (not guard.isNeutral()) {
			guard = popValid(guard);
			if (hasTrue(guard)) {
				result.kind = result.kind == "xuntil" ? "xunless" : "unless";
				guard = popTrue(guard);
			}
			result.guard = export_expression(guard, nets);
			comp.branches.push_back(std::shared_ptr<parse::syntax>(result.clone()));
			return true;
		}

		comp.branches.push_back(std::shared_ptr<parse::syntax>(result.clone()));
		return false;
	}

	arithmetic::Expression guard = isEmbedded and not isGuarded
		? compute_guard(tree, i).minimize()
		: arithmetic::Expression::vdd();
	if (not guard.isValid()) {
		parse_cog::control result;
		result.valid = true;
		result.kind = "await";
		guard = popValid(guard);
		if (hasTrue(guard)) {
			result.kind = "if";
			guard = popTrue(guard);
		}
		result.guard = export_expression(guard, nets);
		result.action = export_composition(tree, i, true, nets);
		comp.branches.push_back(std::shared_ptr<parse::syntax>(result.clone()));
		return false;
	}

	parse_cog::composition result = export_composition(tree, i, false, nets);
	if (comp.level == result.level or result.branches.size() == 1u) {
		comp.branches.insert(comp.branches.end(), result.branches.begin(), result.branches.end());
		comp.comp.insert(comp.comp.end(), result.comp.begin(), result.comp.end());
	} else {
		comp.branches.push_back(std::shared_ptr<parse::syntax>(result.clone()));
	}
	return false;
}

parse_cog::composition export_composition(const controlflow::Tree<chp::transition, chp::place> &tree, controlflow::Index i, bool isGuarded, ucs::ConstNetlist nets) {
	bool isEmbedded = false;
	parse_cog::composition result;
	result.level = parse_cog::composition::SEQUENCE;
	std::string comp = "\n";

	result.valid = true;
	if (tree.nodes[i.index].type == controlflow::Node::CHOICE) {
		result.level = parse_cog::composition::CHOICE;
		comp = "xor";
		if (tree.nodes[i.index].place >= 0) {
			comp = tree.places[tree.nodes[i.index].place].arbiter ? "xor" : "or";
		}
		isEmbedded = true;
	} else if (tree.nodes[i.index].type == controlflow::Node::PARALLEL) {
		result.level = parse_cog::composition::PARALLEL;
		comp = "and";
	}

	if (i.type == controlflow::Index::TRANSITION) {
		export_control(result, tree, i, controlflow::Index(), isEmbedded, isGuarded, nets);
		return result;
	}

	for (size_t j = 0; j < tree.nodes[i.index].procs.size(); j++) {
		if (not result.branches.empty()) {
			result.comp.push_back(comp);
		}

		controlflow::Index curr = tree.nodes[i.index].procs[j];
		controlflow::Index next;
		if (j+1 < tree.nodes[i.index].procs.size()
			and (tree.nodes[i.index].type == controlflow::Node::SEQUENCE
				or tree.nodes[i.index].type == controlflow::Node::LOOP)) {
			next = tree.nodes[i.index].procs[j+1];
		}
		
		if (export_control(result, tree, curr, next, isEmbedded, isGuarded, nets)) {
			j++;
		}

		if (result.level == parse_cog::composition::SEQUENCE) {
			isGuarded = false;
		}
	}
	return result;
}

parse_cog::composition export_cog(const controlflow::Tree<chp::transition, chp::place> &tree, ucs::ConstNetlist nets, controlflow::Index i) {
	parse_cog::composition result;
	result.level = parse_cog::composition::SEQUENCE;
	result.valid = true;

	if (i.index < 0) {
		if (tree.root < 0) {
			return result;
		}
		i.type = controlflow::Index::NODE;
		i.index = tree.root;
	}

	export_control(result, tree, i, controlflow::Index(), false, false, nets);
	return result;
}

// Flow

parse_cog::assignment export_in(int uid, ucs::ConstNetlist nets) {
	return export_assignment(
		arithmetic::Action(
			arithmetic::memberCall(
				arithmetic::Expression::varOf(uid), "recv", {})),
		nets);
}

parse_cog::composition export_ins(const std::vector<int> &ins, ucs::ConstNetlist nets) {
	parse_cog::composition result;
	result.valid = true;
	result.level = parse_cog::composition::PARALLEL;
	for (int in : ins) {
		if (not result.branches.empty()) {
			result.comp.push_back("and");
		}
		result.branches.push_back(std::shared_ptr<parse::syntax>(export_in(in, nets).clone()));
	}
	return result;
}

parse_cog::assignment export_reg(int uid, arithmetic::Expression expr, ucs::ConstNetlist nets) {
	return export_assignment(
		arithmetic::Action(
			arithmetic::Expression::varOf(uid), expr),
		nets);
}

parse_cog::composition export_regs(const std::vector<std::pair<int, arithmetic::Expression> > &regs, ucs::ConstNetlist nets) {
	parse_cog::composition result;
	result.valid = true;
	result.level = parse_cog::composition::PARALLEL;
	for (const auto &reg : regs) {
		if (not result.branches.empty()) {
			result.comp.push_back("and");
		}
		result.branches.push_back(std::shared_ptr<parse::syntax>(export_reg(reg.first, reg.second, nets).clone()));
	}
	return result;
}

parse_cog::assignment export_out(int uid, arithmetic::Expression expr, ucs::ConstNetlist nets) {
	arithmetic::Action action(
		arithmetic::memberCall(
			arithmetic::Expression::varOf(uid),
			"send", {expr}));
	return export_assignment(action, nets);
}

parse_cog::composition export_outs(const std::vector<std::pair<int, arithmetic::Expression> > &outs, ucs::ConstNetlist nets) {
	parse_cog::composition result;
	result.valid = true;
	result.level = parse_cog::composition::PARALLEL;
	for (const auto &out : outs) {
		if (not result.branches.empty()) {
			result.comp.push_back("and");
		}
		result.branches.push_back(std::shared_ptr<parse::syntax>(export_out(out.first, out.second, nets).clone()));
	}
	return result;
}

parse_cog::composition export_branch(const flow::Condition &cond, ucs::ConstNetlist nets) {
	parse_cog::composition result;
	result.valid = true;
	result.level = parse_cog::composition::SEQUENCE;

	if (not cond.outs.empty()) {
		result.branches.push_back(std::shared_ptr<parse::syntax>(export_outs(cond.outs, nets).clone()));
	}
	if (not cond.regs.empty()) {
		if (not result.branches.empty()) {
			result.comp.push_back("\n");
		}
		result.branches.push_back(std::shared_ptr<parse::syntax>(export_regs(cond.regs, nets).clone()));
	}
	if (not cond.ins.empty()) {
		if (not result.branches.empty()) {
			result.comp.push_back("\n");
		}
		result.branches.push_back(std::shared_ptr<parse::syntax>(export_ins(cond.ins, nets).clone()));
	}
	return result;

}

parse_cog::control export_condition(const flow::Condition &cond, ucs::ConstNetlist nets) {
	parse_cog::control result;
	result.valid = true;
	result.kind = "await";
	result.guard = export_expression(cond.valid, nets);
	result.action = export_branch(cond, nets);
	return result;
}

parse_cog::composition export_conditions(const std::vector<flow::Condition> &conds, ucs::ConstNetlist nets) {
	if (conds.size() == 1u and conds[0].valid.isValid()) {
		return export_branch(conds[0], nets);
	}

	parse_cog::composition result;
	result.valid = true;
	result.level = parse_cog::composition::CHOICE;
	for (const auto &cond : conds) {
		if (not result.branches.empty()) {
			result.comp.push_back("or");
		}
		result.branches.push_back(std::shared_ptr<parse::syntax>(export_condition(cond, nets).clone()));
	}
	return result;
}

parse_cog::control export_main(const flow::Func &func) {
	parse_cog::control result;
	result.valid = true;
	result.kind = "while";
	result.guard = export_expression(arithmetic::Expression::boolOf(true), func);
	result.action = export_conditions(func.conds, func);
	return result;
}

parse_cog::assignment export_instance(const flow::Instance &inst, ucs::ConstNetlist nets) {
	return export_assignment(
		arithmetic::Action(
			arithmetic::Expression::undef(),
			arithmetic::call(inst.type, inst.ports)
		), nets);
}

parse_cog::composition export_func(const flow::Func &func) {
	parse_cog::composition result;
	result.valid = true;
	result.level = parse_cog::composition::PARALLEL;
	result.branches.push_back(std::shared_ptr<parse::syntax>(export_main(func).clone()));
	for (const auto &inst : func.inst) {
		result.comp.push_back("and");
		result.branches.push_back(std::shared_ptr<parse::syntax>(export_instance(inst, func).clone()));
	}
	return result;
}


/*parse_chp::sequence export_sequence(vector<petri::iterator> &i, const chp::graph &g, boolean::variable_set &v)
{
	parse_chp::sequence result;
	result.valid = true;

	vector<petri::iterator> choiceed;

	while (1)
	{
		if (i.size() == 1 && i[0].type == chp::transition::type && g.transitions[i[0].index].behavior == chp::transition::active)
			result.actions.push_back(new parse_boolean::internal_choice(export_internal_choice(g.transitions[i[0].index].action, v)));
		else if (i.size() == 1 && i[0].type == chp::transition::type && g.transitions[i[0].index].behavior == chp::transition::passive)
		{
			parse_chp::condition *c = new parse_chp::condition();
			c->valid = true;
			c->branches.resize(1);
			c->branches[0].first = export_disjunction(g.transitions[i[0].index].action, v);
			result.actions.push_back(c);
		}
		else if (i.size() > 1 && i[0].type == chp::place::type)
			result.actions.push_back(new parse_chp::parallel(export_parallel(i, g, v)));
		else if (i.size() > 1 && i[0].type == chp::transition::type)
			result.actions.push_back(new parse_chp::condition(export_condition(i, g, v)));

		vector<petri::iterator> n = g.next(i);
		sort(n.begin(), n.end());
		n.resize(unique(n.begin(), n.end()) - n.begin());

		vector<petri::iterator> p = g.prev(n);
		sort(p.begin(), p.end());
		p.resize(unique(p.begin(), p.end()) - p.begin());


		if (vector_intersection_size(choiceed, p) != 0 || p.size() > i.size())
			return result;
		else
		{
			choiceed.insert(choiceed.end(), i.begin(), i.end());
			i = n;
		}
	}
}

parse_chp::parallel export_parallel(vector<petri::iterator> &i, const chp::graph &g, boolean::variable_set &v)
{
	parse_chp::parallel result;
	result.valid = true;
	vector<petri::iterator> end;

	for (int j = 0; j < (int)i.size(); j++)
	{
		vector<petri::iterator> start(1, i[j]);
		result.branches = export_sequence(start, g, v);
		end.insert(end.end(), start.begin(), start.end());
	}

	i = end;

	return result;
}

parse::syntax *export_condition(vector<petri::iterator> &i, const chp::graph &g, boolean::variable_set &v)
{
	parse_chp::condition result;
	result.valid = true;
	vector<petri::iterator> end;

	for (int j = 0; j < (int)i.size(); j++)
	{
		vector<petri::iterator> start(1, i[j]);
		result.branches.push_back(pair<parse_boolean::disjunction, parse_chp::parallel>());
		result.branches.back().second.valid = true;
		parse_chp::sequence s = export_sequence(start, g, v);
		if (s.actions.size() > 0 && s.actions[0]->is_a<parse_chp::condition>() &&
			((parse_chp::condition*)s.actions[0])->branches.size() == 1 &&
			((parse_chp::condition*)s.actions[0])->branches.back().second.branches.size() == 0)
		{
			result.branches.back().first = ((parse_chp::condition*)s.actions[0])->branches.back().first;
			delete s.actions[0];
			s.actions.erase(s.actions.begin());
		}
		else
			result.branches.back().first = export_disjunction(boolean::choice(boolean::parallel()), v);

		result.branches.back().second.branches.push_back(s);

		end.insert(end.end(), start.begin(), start.end());
	}

	i = end;

	return result;
}*/

/*
parse_chp::sequence export_sequence(vector<petri::iterator> nodes, map<petri::iterator, int> counts, const chp::graph &g, const boolean::variable_set &v)
{
	// Maintain a stack to help us manage the hierarchy. The deeper
	// the stack, the more hierarchy there is. This stack stores
	// only sequences. Since we know at this point that every parallel
	// or conditional block we introduce will only have one branch, we
	// can get away with this. We will merge these sequences later to
	// add parallelism or choice.
	parse_chp::sequence head;
	head.valid = true;
	vector<parse_chp::sequence*> stack;
	stack.push_back(&head);

	int delta = 0;
	int value = counts[nodes[0]]-1;
	petri::iterator last = nodes[0];
	for (int j = 1; j < (int)nodes.size(); j++)
	{
		int c = counts[nodes[j]]-1;
		delta = c - value;
		value = c;

		// The count increased, we need to remove hierarchy
		if (delta > 0 && j > 1)
		{
			if (stack.size() == 0)
			{
				error("", "chp not properly nested", __FILE__, __LINE__);
				return parse_chp::sequence();
			}

			stack.back()->end = last.index;

			if (stack.size() > 1)
				stack[stack.size()-2]->actions.back()->end = nodes[j].index;

			stack.pop_back();
			delta--;
		}

		// The count decreased, we need to add hierarchy
		if (delta < 0)
		{
			// The last node before the count decrease was a transition, meaning
			// we need to wrap the next couple transitions in a parallel block
			if (last.type == chp::transition::type)
			{
				parse_chp::parallel *tmp = new parse_chp::parallel();
				tmp->valid = true;

				// This is very important: we need to keep track of what
				// syntaxes belong to what nodes. That way we can compare them
				// later when we go to merge them.
				tmp->start = last.index;

				parse_chp::sequence new_head;
				new_head.valid = true;
				new_head.start = nodes[j].index;
				tmp->branches.push_back(new_head);

				stack.back()->actions.push_back(tmp);
				stack.push_back(&tmp->branches.back());
			}
			// The last node before the count decrease was a place, meaning we need
			// to wrap the next couple transitions in a conditional block. Never
			// mind the guard, we will take care of that later.
			else if (last.type == chp::place::type)
			{
				parse_chp::condition *tmp = new parse_chp::condition();
				tmp->valid = true;

				// This is very important: we need to keep track of what
				// syntaxes belong to what nodes. That way we can compare them
				// later when we go to merge them.
				tmp->start = last.index;

				tmp->branches.push_back(pair<parse_boolean::guard, parse_chp::parallel>());
				tmp->branches.back().second.valid = true;
				parse_chp::sequence new_head;
				new_head.valid = true;
				new_head.start = nodes[j].index;
				tmp->branches.back().second.branches.push_back(new_head);

				stack.back()->actions.push_back(tmp);
				stack.push_back(&tmp->branches.back().second.branches.back());
			}

			delta++;
		}

		// Once we have dealt with the hierarchy issues, we still need to add assignments and disjunctions into the chp block.
		// We'll package the disjunctions into conditionals later.
		if (stack.size() == 0)
		{
			internal("", "empty stack", __FILE__, __LINE__);
			return head;
		}
		else if (nodes[j].type == chp::transition::type && g.transitions[nodes[j].index].behavior == chp::transition::active)
		{
			stack.back()->actions.push_back(new parse_boolean::assignment(export_composition(g.transitions[nodes[j].index].action, v)));
			stack.back()->actions.back()->start = nodes[j].index;
			stack.back()->actions.back()->end = nodes[j].index;
		}
		else if (nodes[j].type == chp::transition::type && g.transitions[nodes[j].index].behavior == chp::transition::passive)
		{
			stack.back()->actions.push_back(new parse_boolean::guard(export_expression_xfactor(g.transitions[nodes[j].index].action, v)));
			stack.back()->actions.back()->start = nodes[j].index;
			stack.back()->actions.back()->end = nodes[j].index;
		}

		last = nodes[j];
	}

	return head;
}


// TODO this doesn't handle the case where one sequence is a subset of another.
bool merge_sequences(parse_chp::sequence &s0, parse_chp::sequence &s1, vector<parse::syntax*> &m)
{
	int offset = 0;
	bool equal = false;
	if (s0.actions.size() >= s1.actions.size())
	{
		for (offset = 0; offset < (int)s0.actions.size() && !equal; offset++)
		{
			equal = true;
			for (int k = 0; k < (int)s1.actions.size() && equal; k++)
				if (s0.actions[(offset + k)%s0.actions.size()]->start != s1.actions[k]->start)
					equal = false;
		}
	}
	else if (s1.actions.size() > s0.actions.size())
	{
		for (offset = 0; offset < (int)s1.actions.size() && !equal; offset++)
		{
			equal = true;
			for (int k = 0; k < (int)s0.actions.size() && equal; k++)
				if (s1.actions[(offset + k)%s1.actions.size()]->start != s0.actions[k]->start)
					equal = false;
		}

		if (equal)
			swap(s0, s1);
	}

	offset--;

	for (int k = 0; k < (int)s1.actions.size() && equal; k++)
	{
		if (s0.actions[(offset+k)%s0.actions.size()]->is_a<parse_chp::parallel>() &&
			s1.actions[k]->is_a<parse_chp::parallel>())
		{
			parse_chp::parallel *tmp0 = (parse_chp::parallel *)s0.actions[(offset+k)%s0.actions.size()];
			parse_chp::parallel *tmp1 = (parse_chp::parallel *)s1.actions[k];
			tmp0->branches.insert(tmp0->branches.end(), tmp1->branches.begin(), tmp1->branches.end());
			m.push_back(tmp0);
		}
		else if (s0.actions[(offset+k)%s0.actions.size()]->is_a<parse_chp::condition>() &&
				s1.actions[k]->is_a<parse_chp::condition>())
		{
			parse_chp::condition *tmp0 = (parse_chp::condition *)s0.actions[(offset+k)%s0.actions.size()];
			parse_chp::condition *tmp1 = (parse_chp::condition *)s1.actions[k];
			tmp0->branches.insert(tmp0->branches.end(), tmp1->branches.begin(), tmp1->branches.end());
			m.push_back(tmp0);
		}
	}

	return equal;
}

// TODO this doesn't know how to handle non-deterministic choice yet.
// The result is that all non-deterministic conditionals are converted to deterministic ones.
// TODO This function does not always throw an error when an HSE is not properly nested.
// It will just return a bunch of independent processes that don't correctly implement the HSE.
parse_chp::parallel export_parallel(const chp::graph &g, const boolean::variable_set &v)
{
	// First step is to identify all of the cycles in the graph.
	vector<vector<petri::iterator> > cycles = g.cycles();

	for (int i = 0; i < (int)cycles.size(); i++)
	{
		printf("{");
		for (int j = 0; j < (int)cycles[i].size(); j++)
		{
			if (j != 0)
				printf(", ");
			printf("%c%d", cycles[i][j].type == chp::place::type ? 'P' : 'T', cycles[i][j].index);
		}
		printf("}\n");
	}

	// We then need to figure out where add hierarchy. We can do this by looking at the
	// number of cycles that share a given node. If that number decreases, then we need
	// to increase the hierarchy level. If that number increases, then we need to decrease
	// the hierarchy level. Knowing this, we can just iterate over all the nodes in each
	// cycle and add either a condition or a parallel depending upon the type of the last
	// node we passed.
	map<petri::iterator, int> counts;
	for (int i = 0; i < (int)cycles.size(); i++)
		count_elements(cycles[i], counts);

	parse_chp::parallel wrapper;
	wrapper.valid = true;
	for (int i = 0; i < (int)cycles.size(); i++)
	{
		// Put all of the generated sequences into one parallel block. Remember that
		// these sequences still need to be wrapped in a loop, but we'll do that in a later step.
		wrapper.branches.push_back(export_sequence(cycles[i], counts, g, v));
	}

	//printf("%s\n", wrapper.to_string().c_str());

	// Our next step is to merge corresponding sequences in this wrapper.
	// If the HSE is properly nested then we can merge two sequences if
	// one is a subset or equals another. When we run up against a conditional
	// or parallel block that exists in both, we merge them, and we can just
	// ignore everything else. This process needs to happen recursively.
	// (though the data recursion is not implemented as a function recursion)
	vector<parse::syntax*> m;
	m.push_back(&wrapper);

	while (m.size() > 0)
	{
		sort(m.begin(), m.end());
		m.resize(unique(m.begin(), m.end()) - m.begin());
		parse::syntax *syn = m.back();
		m.pop_back();

		if (syn->is_a<parse_chp::parallel>())
		{
			parse_chp::parallel *par = (parse_chp::parallel *)syn;

			for (int i = 0; i < (int)par->branches.size(); i++)
				for (int j = (int)par->branches.size()-1; j >= i+1; j--)
					if (merge_sequences(par->branches[i], par->branches[j], m))
						par->branches.erase(par->branches.begin() + j);
		}
		else if (syn->is_a<parse_chp::condition>())
		{
			parse_chp::condition *par = (parse_chp::condition *)syn;

			for (int i = 0; i < (int)par->branches.size(); i++)
				for (int j = (int)par->branches.size()-1; j >= i+1; j--)
					if (merge_sequences(par->branches[i].second.branches[0], par->branches[j].second.branches[0], m))
						par->branches.erase(par->branches.begin() + j);
		}
	}

	//printf("%s\n", wrapper.to_string().c_str());

	// Next, we need to fix the syntax for all of the conditionals by finding their appropriate disjunctions
	// and moving them to the right slots. We also wrap guards in their own conditional here. This process
	// requires recursion (again, not functionally implemented).
	m.push_back(&wrapper);
	while(m.size() > 0)
	{
		parse::syntax *syn = m.back();
		m.pop_back();

		// We've run up against a conditional
		if (syn->is_a<parse_chp::condition>())
		{
			parse_chp::condition *c = (parse_chp::condition*)syn;
			for (int i = 0; i < (int)c->branches.size(); i++)
			{
				// Check to see if the first item in the sequence of this branch is a disjunction
				if (c->branches[i].second.branches.size() == 1 && c->branches[i].second.branches[0].actions.size() > 0 && c->branches[i].second.branches[0].actions[0]->is_a<parse_boolean::guard>())
				{
					// if it is, we can move it to the guard
					c->branches[i].first = *(parse_boolean::guard*)c->branches[i].second.branches[0].actions[0];
					delete c->branches[i].second.branches[0].actions[0];
					c->branches[i].second.branches[0].actions.erase(c->branches[i].second.branches[0].actions.begin());
				}
				else
					// if it isn't then we need to write our own guard.
					c->branches[i].first = export_expression(boolean::parallel(), v);

				// recurse
				m.push_back(&c->branches[i].second);
			}
		}
		// parallel block, we just recurse here
		else if (syn->is_a<parse_chp::parallel>())
		{
			parse_chp::parallel *p = (parse_chp::parallel*)syn;
			for (int i = 0; i < (int)p->branches.size(); i++)
				m.push_back(&p->branches[i]);
		}
		// A sequence, find all of the disjunctions that don't belong to conditionals
		// and give them their own conditional. Then, recurse.
		else if (syn->is_a<parse_chp::sequence>())
		{
			parse_chp::sequence *s = (parse_chp::sequence*)syn;
			for (int i = 0; i < (int)s->actions.size(); i++)
			{
				if (s->actions[i]->is_a<parse_boolean::guard>())
				{
					parse_chp::condition *c = new parse_chp::condition();
					c->valid = true;
					c->deterministic = true;
					c->branches.resize(1);
					c->branches.back().first = *((parse_boolean::guard*)s->actions[i]);
					c->start = c->branches.back().first.start;
					delete s->actions[i];
					s->actions[i] = c;
				}
				else
					m.push_back(s->actions[i]);
			}
		}
	}

	//printf("%s\n", wrapper.to_string().c_str());

	// Finally, just wrap all of the sequences in infinite loops and call it a day.
	for (int i = 0; i < (int)wrapper.branches.size(); i++)
	{
		parse_chp::loop *l = new parse_chp::loop();
		l->valid = true;
		l->branches.resize(1);
		l->branches[0].second.branches.resize(1);
		l->branches[0].second.branches[0] = wrapper.branches[i];
		wrapper.branches[i].clear();
		wrapper.branches[i].actions.push_back(l);
	}

	//printf("%s\n", wrapper.to_string().c_str());

	return wrapper;
}*/

}
