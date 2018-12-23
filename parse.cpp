/*
 * parse.cpp
 */

#include "parse.h"

// WRAPPER FOR PUSHBACK
namespace Parser {
bool pushed_back = false;
Token	pushed_token;

static Token GetNextToken(istream *in, int *line) {
	if( pushed_back ) {
		pushed_back = false;
		return pushed_token;
	}
	return getNextToken(in, line);
}

static void PushBackToken(Token& t) {
	if( pushed_back ) {
		abort();
	}
	pushed_back = true;
	pushed_token = t;
}

}

static int error_count = 0;

void
ParseError(int line, string msg)
{
	++error_count;
	cout << line << ": " << msg << endl;
}

ParseTree *Prog(istream *in, int *line) //Stmt SC { Slist }
{
	ParseTree *sl = Slist(in, line);

	if( sl == 0 )
		ParseError(*line, "No statements in program");

	if( error_count )
		return 0;

	return sl;
}

// Slist is a Statement followed by a Statement List
ParseTree *Slist(istream *in, int *line) { //Stmt SC { Slist }
	ParseTree *s = Stmt(in, line);
	if( s == 0 )
		return 0;

	if( Parser::GetNextToken(in, line) != SC ) {
		ParseError(*line, "Missing semicolon");
		return 0;
	}

    return new StmtList(s, Slist(in,line));
}

ParseTree *Stmt(istream *in, int *line) { //IfStmt | PrintStmt | Expr
	ParseTree *s;

	Token t = Parser::GetNextToken(in, line);
	switch( t.GetTokenType() ) {
	case IF:
		s = IfStmt(in, line);
		break;

	case PRINT:
		s = PrintStmt(in, line);
		break;

	case DONE:
		return 0;

	case ERR:
		ParseError(*line, "Invalid token");
		return 0;

	default:
		// put back the token and then see if it's an Expr
		Parser::PushBackToken(t);
		s = Expr(in, line);
		if( s == 0 ) {
			ParseError(*line, "Invalid statement");
			return 0;
		}
		break;
	}


	return s;
}

ParseTree *IfStmt(istream *in, int *line) { //IF Expr THEN Stmt
    
    ParseTree *ex = Expr(in, line);   
    Token t = Parser::GetNextToken(in, line);
    if(t.GetTokenType() == THEN){
        ParseTree *stmt = Stmt(in, line);
        return new IfStatement(t.GetLinenum(), ex, stmt);
    }
    
    return 0;
    //return new IfStatement(t.GetLinenum(), ex, stmt);
}


ParseTree *PrintStmt(istream *in, int *line) { //PRINT Expr
	int l = *line;
	ParseTree *ex = Expr(in, line);
    return new PrintStatement(l, ex);
}

ParseTree *Expr(istream *in, int *line) { //LogicExpr{ ASSIGN LogicExpr }
	ParseTree *t1 = LogicExpr(in, line);
	if( t1 == 0 ) {
		return 0;
	}

	Token t = Parser::GetNextToken(in, line);

	if( t != ASSIGN ) {
		Parser::PushBackToken(t);
		return t1;
	}

	ParseTree *t2 = Expr(in, line); // right assoc
	if( t2 == 0 ) {
		ParseError(*line, "Missing expression after operator");
		return 0;
	}

	return new Assignment(t.GetLinenum(), t1, t2);
}

ParseTree *LogicExpr(istream *in, int *line) { //CompareExpr { (LOGICAND | LOGICOR) CompareExpr }
	ParseTree *t1 = CompareExpr(in, line);
    
    while ( true ) {
		Token t = Parser::GetNextToken(in, line);

		if( t != LOGICAND && t != LOGICOR ) {
			Parser::PushBackToken(t);
			return t1;
		}

		ParseTree *t2 = CompareExpr(in, line);
		
		if( t == LOGICAND)
			t1 = new LogicAndExpr(t.GetLinenum(), t1, t2);
		else
			t1 = new LogicOrExpr(t.GetLinenum(), t1, t2);
	}
    
    // HANDLE OP
    return 0;
}

ParseTree *CompareExpr(istream *in, int *line) { //AddExpr { (EQ | NEQ | GT | GEQ | LT | LEQ) AddExpr }
    ParseTree *t1 = AddExpr(in, line);
	if( t1 == 0 ) {
		return 0;
	}
    
    while ( true ) {
		Token t = Parser::GetNextToken(in, line);
               
		if( t.GetTokenType() != EQ && t.GetTokenType() != NEQ && t.GetTokenType() != GT && t.GetTokenType() != GEQ && t.GetTokenType() != LT && t.GetTokenType() != LEQ) {
			Parser::PushBackToken(t);
			return t1;
		}

		ParseTree *t2 = AddExpr(in, line);
		       
        switch(t.GetTokenType()){
            case EQ:
                t1 = new EqExpr(t.GetLinenum(), t1, t2);
                break;
            case GT:
                t1 = new GtExpr(t.GetLinenum(), t1, t2);
                break;
            case NEQ:
                t1 = new NEqExpr(t.GetLinenum(), t1, t2);
                break;
            case GEQ:
                t1 = new GEqExpr(t.GetLinenum(), t1, t2);
                break;
            case LT:
                t1 = new LtExpr(t.GetLinenum(), t1, t2);
                break;
            case LEQ:
                t1 = new LEqExpr(t.GetLinenum(), t1, t2);
                break;
            default:
                return 0;
                break;
        }
	}
    
    // HANDLE OP
    return 0;
}

ParseTree *AddExpr(istream *in, int *line) { //MulExpr { (PLUS | MINUS) MulExpr }
	ParseTree *t1 = MulExpr(in, line);
	if( t1 == 0 ) {
		return 0;
	}

	while ( true ) {
		Token t = Parser::GetNextToken(in, line);

		if( t != PLUS && t != MINUS ) {
			Parser::PushBackToken(t);
			return t1;
		}

		ParseTree *t2 = MulExpr(in, line);
		if( t2 == 0 ) {
			ParseError(*line, "Missing expression after operator");
			return 0;
		}

		if( t == PLUS )
			t1 = new PlusExpr(t.GetLinenum(), t1, t2);
		else
			t1 = new MinusExpr(t.GetLinenum(), t1, t2);
	}
}

ParseTree *MulExpr(istream *in, int *line) { //Factor { (STAR | SLASH) Factor }
	ParseTree *t1 = Factor(in, line);
	if( t1 == 0 ) {
		return 0;
	}
    
    while ( true ) {
		Token t = Parser::GetNextToken(in, line);

		if( t.GetTokenType() != STAR && t.GetTokenType() != SLASH ) {
			Parser::PushBackToken(t);
			return t1;
		}

		ParseTree *t2 = Factor(in, line);

		if( t.GetTokenType() == STAR){t1 = new TimesExpr(t.GetLinenum(), t1, t2);}
		else{t1 = new DivideExpr(t.GetLinenum(), t1, t2);}
	}
    
    // HANDLE OP
    return 0;
}

ParseTree *Factor(istream *in, int *line) { //MINUS Primary | Primary
	bool neg = false;
	Token t = Parser::GetNextToken(in, line);

	if( t == MINUS ) {
		neg = true;
	}
	else {
		Parser::PushBackToken(t);
	}

	ParseTree *p1 = Primary(in, line);
	if( p1 == 0 ) {
		ParseError(*line, "Missing primary");
		return 0;
	}

	if( neg ) {
        // handle as -1 * Primary
		return new TimesExpr(t.GetLinenum(), new IConst(t.GetLinenum(), -1), p1);
	}
	else
		return p1;
}

ParseTree *Primary(istream *in, int *line) { //IDENT | ICONST | SCONST | TRUE | FALSE | LPAREN Expr // RPAREN
	Token t = Parser::GetNextToken(in, line);

	if( t.GetTokenType() == IDENT ) {
		return new Ident(t);
	}
	else if( t.GetTokenType() == ICONST ) {
		return new IConst(t);
	}
	else if( t.GetTokenType() == SCONST ) {
		return new SConst(t);
	}
    else if(t.GetTokenType() == TRUE){
        return new BoolConst(t, true);
    }
    else if(t.GetTokenType() == FALSE){
        return new BoolConst(t, false);
    }
	else if( t.GetTokenType() == LPAREN ) {
		ParseTree *ex = Expr(in, line);       
		if( Parser::GetNextToken(in, line) == RPAREN ){return ex;}
		return 0;
	}
    
	return 0;

    // PROCESS TOKEN, IDENTIFY PRIMARY, RETURN SOMETHING
}
