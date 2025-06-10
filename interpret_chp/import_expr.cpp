#include "import_expr.h"
#include <interpret_arithmetic/import.h>

namespace chp {

petri::segment import_segment(chp::graph &dst, const parse_expression::expression &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	petri::iterator t = dst.create(chp::transition(arithmetic::import_expression(syntax, dst, default_id, tokens, auto_define), arithmetic::Choice(arithmetic::Parallel())));
	return petri::segment({{t}}, {{t}});
}

petri::segment import_segment(chp::graph &dst, const parse_expression::assignment &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	petri::iterator t = dst.create(chp::transition(arithmetic::Expression(true), arithmetic::import_action(syntax, dst, default_id, tokens, auto_define)));
	return petri::segment({{t}}, {{t}});
}

}
