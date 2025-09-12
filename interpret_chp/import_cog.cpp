#include "import_cog.h"
#include "import_expr.h"

#include <common/standard.h>
#include <interpret_arithmetic/import.h>
#include "export_astg.h"

namespace chp {

segment import_segment(chp::graph &dst, const parse_cog::composition &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	bool arbiter = false;
	bool synchronizer = false;

	int composition = petri::parallel;
	if (syntax.level == parse_cog::composition::SEQUENCE or syntax.level == parse_cog::composition::INTERNAL_SEQUENCE) {
		composition = petri::sequence;
	} else if (syntax.level == parse_cog::composition::CONDITION) {
		composition = petri::choice;
	} else if (syntax.level == parse_cog::composition::CHOICE) {
		composition = petri::choice;
		arbiter = true;
	} else if (syntax.level == parse_cog::composition::PARALLEL) {
		composition = petri::parallel;
	}

	segment result(composition != petri::choice);
	for (int i = 0; i < (int)syntax.branches.size(); i++) {
		segment branch(true);
		if (syntax.branches[i].sub != nullptr and syntax.branches[i].sub->valid) {
			branch = import_segment(dst, *syntax.branches[i].sub, default_id, tokens, auto_define);
		} else if (syntax.branches[i].ctrl != nullptr and syntax.branches[i].ctrl->valid) {
			branch = import_segment(dst, *syntax.branches[i].ctrl, default_id, tokens, auto_define);
		} else if (syntax.branches[i].assign.valid) {
			branch = import_segment(dst, syntax.branches[i].assign, default_id, tokens, auto_define);
		} else {
			continue;
		}
		result = compose(dst, composition, result, branch);
	}

	if (syntax.branches.size() == 0) {
		petri::iterator from = dst.create(chp::place());

		result.nodes.source.push_back({from});
		result.nodes.sink.push_back({from});
	}

	// At this point, source and sink are likely *transitions*.
	// If this is a conditional composition, then they need to be
	// the places *before* those transitions. And if there aren't
	// any places before those transitions then we need to create
	// them

	if (result.nodes.source.size() > 1u and composition == choice) {
		petri::iterator from = dst.create(chp::place());
		dst.connect({{from}}, result.nodes.source);
		result.nodes.source = petri::bound({{from}});
	}

	if (result.nodes.sink.size() > 1u and composition == choice) {
		petri::iterator to = dst.create(chp::place());
		dst.connect(result.nodes.sink, {{to}});
		result.nodes.sink = petri::bound({{to}});
	}

	/*if ((int)syntax.branches.size() > 1) {
		cout << "BEFORE" << endl;
		cout << syntax.to_string() << endl;
		cout << result << endl << endl;

		cout << export_astg(dst).to_string() << endl << endl;
	}*/

	if ((arbiter or synchronizer) and syntax.branches.size() > 1u) {
		for (auto i = result.nodes.source.begin(); i != result.nodes.source.end(); i++) {
			for (auto j = i->begin(); j != i->end(); j++) {
				if (j->type != place::type) {
					printf("%s:%d: internal: expected place\n", __FILE__, __LINE__);
				} else {
					if (arbiter) {
						dst.places[j->index].arbiter = true;
					}
					if (synchronizer) {
						//dst.places[j->index].synchronizer = true;
					}
				}
			}
		}
	}

	if (result.loop and composition == choice and not arbiter and not result.nodes.source.empty()) {
		arithmetic::Expression skipCond = ~result.cond;
		if (not skipCond.isNull()) {
			petri::iterator arrow = dst.create(chp::place());
			for (auto i = result.nodes.source.begin(); i != result.nodes.source.end(); i++) {
				petri::iterator skip = dst.create(chp::transition(skipCond));
				dst.connect(*i, skip);
				dst.connect(skip, arrow);
			}

			dst.connect(result.nodes.sink, {{arrow}});
			result.nodes.sink = petri::bound({{arrow}});
		}
		result.loop = false;
	}

	/*if ((int)syntax.branches.size() > 1) {
		cout << "AFTER" << endl;
		cout << result << endl << endl;

		cout << export_astg(dst).to_string() << endl << endl;
	}*/

	return result;
}

segment import_segment(chp::graph &dst, const parse_cog::control &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	if (syntax.region != "") {
		default_id = atoi(syntax.region.c_str());
	}

	segment result(true);
	if (syntax.guard.valid) {
		segment sub = import_segment(dst, syntax.guard, default_id, tokens, auto_define);
		if (syntax.kind == "if") {
			sub.cond = arithmetic::isTrue(sub.cond);
		} else if (syntax.kind == "await") {
			sub.cond = arithmetic::isValid(sub.cond);
		}
		result = compose(dst, petri::sequence, result, sub);
	}
	if (syntax.action.valid) {
		segment sub = import_segment(dst, syntax.action, default_id, tokens, auto_define);
		result = compose(dst, petri::sequence, result, sub);
	}

	if (syntax.kind == "while" and not result.nodes.source.empty()) {
		result.loop = true;
		petri::iterator link = dst.create(chp::place());
		dst.connect({{link}}, result.nodes.source);
		dst.connect(result.nodes.sink, {{link}});
		result.nodes.sink.clear();
		if (not result.nodes.reset.empty()) {
			result.nodes.source = result.nodes.reset;
			result.nodes.reset.clear();
		} else {
			result.nodes.source = petri::bound({{link}});
		}
	}

	return result;
}

void import_chp(chp::graph &dst, const parse_cog::composition &syntax, tokenizer *tokens, bool auto_define) {
	petri::segment result = chp::import_segment(dst, syntax, 0, tokens, auto_define).nodes;
	if (not result.reset.empty()) {
		result.source = result.reset;
		result.reset.clear();
	}

	vector<chp::state> reset;
	for (auto i = result.source.begin(); i != result.source.end(); i++) {
		chp::state rst;
		for (auto j = i->begin(); j != i->end(); j++) {
			if (j->type == chp::transition::type) {
				vector<petri::iterator> p = dst.prev(*j);
				if (p.empty()) {
					p.push_back(dst.create(place()));
					dst.connect(p.back(), *j);
				}
				for (auto k = p.begin(); k != p.end(); k++) {
					rst.tokens.push_back(petri::token(k->index));
				}
			} else {
				rst.tokens.push_back(petri::token(j->index));
			}
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
				dst.reset.push_back(state::merge(*i, *j));
			}
		}
	}
}

}
