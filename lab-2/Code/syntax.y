%locations
%{
  #include "lex.yy.c"
  extern void yyerror(const char* msg);
  extern int has_error;

  #ifdef SYNTAX_DEBUG
  #define error_end(err)  \
    printf("error '%s' ends at Line %d;", err, yylineno);
  #else
  #define error_end(err)
  #endif
%}

/* tokens */
%token INT 
%token FLOAT
%token SEMI COMMA
%token ASSIGNOP RELOP PLUS MINUS STAR DIV
%token AND OR DOT NOT
%token TYPE ID
%token LP RP LB RB LC RC
%token STRUCT RETURN IF ELSE WHILE

/* precedence and associativity */
%nonassoc LTE
%nonassoc ELSE

%right ASSIGNOP
%left  OR
%left  AND
%left  RELOP
%left  PLUS MINUS
%left  STAR DIV
%right NEG NOT
%nonassoc NORPRB
%nonassoc LP RP LB RB DOT

%%
/* rules */
/* High-level Definitions */
Program : ExtDefList  { $$ = gen_tree("Program", PROGRAM, @$.first_line, 1, $1); tree = $$; }
  ;
ExtDefList : ExtDef ExtDefList  { $$ = gen_tree("ExtDefList", EXT_DEF_LIST, @$.first_line, 2, $1, $2); }
  | /* empty */                 { $$ = gen_tree("ExtDefList", EXT_DEF_LIST, yylineno, 0); }
  ;
ExtDef : Specifier ExtDecList SEMI      { $$ = gen_tree("ExtDef", EXT_DEF, @$.first_line, 3, $1, $2, $3); }
  | Specifier SEMI                      { $$ = gen_tree("ExtDef", EXT_DEF, @$.first_line, 2, $1, $2); }
  | Specifier FunDec CompSt             { $$ = gen_tree("ExtDef", EXT_DEF, @$.first_line, 3, $1, $2, $3); }
  | Specifier FunDec SEMI /* LAB_2_1 */ { $$ = gen_tree("ExtDef", EXT_DEF, @$.first_line, 3, $1, $2, $3); }
  /* E */ | error SEMI                      { error_end("ExtDef -> error SEMI"); yyerrok; }
  ;
ExtDecList : VarDec           { $$ = gen_tree("ExtDecList", EXT_DEC_LIST, @$.first_line, 1, $1); }
  | VarDec COMMA ExtDecList   { $$ = gen_tree("ExtDecList", EXT_DEC_LIST, @$.first_line, 3, $1, $2, $3); }
  ;
/* Specifiers */
Specifier : TYPE      { $$ = gen_tree("Specifier", SPECIFIER, @$.first_line, 1, $1); }
  | StructSpecifier   { $$ = gen_tree("Specifier", SPECIFIER, @$.first_line, 1, $1); }
  ;
StructSpecifier : STRUCT OptTag LC DefList RC   { $$ = gen_tree("StructSpecifier", STRUCT_SPECIFIER, @$.first_line, 5, $1, $2, $3, $4, $5); }
  | STRUCT Tag                                  { $$ = gen_tree("StructSpecifier", STRUCT_SPECIFIER, @$.first_line, 2, $1, $2); }
  /* E */ | STRUCT error LC DefList RC                  { error_end("StructSpecifier -> STRUCT error LC DefList RC"); }
  ;
OptTag : ID       { $$ = gen_tree("OptTag", OPT_TAG, @$.first_line, 1, $1); }
  | /* empty */   { $$ = gen_tree("OptTag", OPT_TAG, yylineno, 0); }
  ;
Tag : ID    { $$ = gen_tree("Tag", TAG, @$.first_line, 1, $1); }
  ;
/* Declarators */
VarDec : ID                           { $$ = gen_tree("VarDec", VAR_DEC, @$.first_line, 1, $1); }
  | VarDec LB INT RB                  { $$ = gen_tree("VarDec", VAR_DEC, @$.first_line, 4, $1, $2, $3, $4); }
  /* E */ | error COMMA                       { error_end("VarDec -> error [COMMA]"); repeat = COMMA; }
  /* E */ | error SEMI                        { error_end("VarDec -> error [SEMI]"); repeat = SEMI; yyerrok; }
  /* E */ | VarDec LB error RB                { error_end("VarDec -> VarDec LB error RB"); }
  /* E */ | VarDec LB error COMMA             { error_end("VarDec -> VarDec LB error [COMMA]"); repeat = COMMA; }
  /* E */ | VarDec LB error SEMI              { error_end("VarDec -> VarDec LB error [SEMI]"); repeat = SEMI; yyerrok; }
  ;
FunDec : ID LP VarList RP   { $$ = gen_tree("FunDec", FUN_DEC, @$.first_line, 4, $1, $2, $3, $4); }
  | ID LP RP                { $$ = gen_tree("FunDec", FUN_DEC, @$.first_line, 3, $1, $2, $3); }
  /* E */ | error LP VarList RP     { error_end("FunDec -> error LP VarList RP"); }
  /* E */ | error LP RP             { error_end("FunDec -> error LP RP"); }
  /* E */ | ID LP error LC          { error_end("FunDec -> ID error [LC]"); repeat = LC; }
  ;
VarList : ParamDec COMMA VarList  { $$ = gen_tree("VarList", VAR_LIST, @$.first_line, 3, $1, $2, $3); }
  | ParamDec                      { $$ = gen_tree("VarList", VAR_LIST, @$.first_line, 1, $1); }
  ;
ParamDec : Specifier VarDec   { $$ = gen_tree("ParamDec", PARAM_DEC, @$.first_line, 2, $1, $2); }
  /* E */ | error COMMA               { error_end("ParamDec -> error [COMMA]"); repeat = COMMA;}
  ;
/* Statements */
CompSt : LC DefList StmtList RC   { $$ = gen_tree("CompSt", COMP_ST, @$.first_line, 4, $1, $2, $3, $4); }
  ;
StmtList : Stmt StmtList  { $$ = gen_tree("StmtList", STMT_LIST, @$.first_line, 2, $1, $2); }
  | /* empty */           { $$ = gen_tree("StmtList", STMT_LIST, yylineno, 0); }
  ;
Stmt : Exp SEMI                     { $$ = gen_tree("Stmt", STMT, @$.first_line, 2, $1, $2); }
  | CompSt                          { $$ = gen_tree("Stmt", STMT, @$.first_line, 1, $1); }
  | RETURN Exp SEMI                 { $$ = gen_tree("Stmt", STMT, @$.first_line, 3, $1, $2, $3); }
  | IF LP Exp RP Stmt %prec LTE     { $$ = gen_tree("Stmt", STMT, @$.first_line, 5, $1, $2, $3, $4, $5); }
  | IF LP Exp RP Stmt ELSE Stmt     { $$ = gen_tree("Stmt", STMT, @$.first_line, 7, $1, $2, $3, $4, $5, $6, $7); }
  | WHILE LP Exp RP Stmt            { $$ = gen_tree("Stmt", STMT, @$.first_line, 5, $1, $2, $3, $4, $5); }
  /* E */ | RETURN error SEMI               { error_end("Stmt -> RETURN error SEMI"); yyerrok; }
  /* E */ | IF LP error RP Stmt %prec LTE   { error_end("Stmt -> IF LP error RP Stmt"); }
  /* E */ | IF LP error Stmt %prec LTE      { error_end("Stmt -> IF LP error Stmt"); }
  /* E */ | IF LP error RP Stmt ELSE Stmt   { error_end("Stmt -> IF LP error RP Stmt ELSE Stmt"); }
  /* E */ | IF LP error Stmt ELSE Stmt      { error_end("Stmt -> IF LP Exp error Stmt ELSE Stmt"); }
  /* E */ | WHILE LP error RP Stmt          { error_end("Stmt -> WHILE LP error RP Stmt"); }
  /* E */ | WHILE LP error Stmt             { error_end("Stmt -> WHILE LP error Stmt"); }
  ;
/* Local Definitions */
DefList : Def DefList   { $$ = gen_tree("DefList", DEF_LIST, @$.first_line, 2, $1, $2); }
  | /* empty */         { $$ = gen_tree("DefList", DEF_LIST, yylineno, 0); }
  ;
Def : Specifier DecList SEMI  { $$ = gen_tree("Def", DEF, @$.first_line, 3, $1, $2, $3); }
  /* E */ | Specifier error SEMI      { error_end("Def -> Specifier error SEMI"); yyerrok; }
  /* E */ | error SEMI                { error_end("Def -> error SEMI"); yyerrok; }
  ;
DecList : Dec             { $$ = gen_tree("DecList", DEC_LIST, @$.first_line, 1, $1); }
  | Dec COMMA DecList     { $$ = gen_tree("DecList", DEC_LIST, @$.first_line, 3, $1, $2, $3); }
  ;
Dec : VarDec              { $$ = gen_tree("Dec", DEC, @$.first_line, 1, $1); }
  | VarDec ASSIGNOP Exp   { $$ = gen_tree("Dec", DEC, @$.first_line, 3, $1, $2, $3); }
  /* E */ | VarDec ASSIGNOP COMMA { yyerror("Expected expression"); repeat = COMMA; YYERROR; }
  /* E */ | VarDec ASSIGNOP SEMI  { yyerror("Expected expression"); repeat = SEMI; YYERROR; }
  ;
/* Expressions */
Exp : Exp ASSIGNOP Exp          { $$ = gen_tree("Exp", EXP, @$.first_line, 3, $1, $2, $3); }
  | Exp AND Exp                 { $$ = gen_tree("Exp", EXP, @$.first_line, 3, $1, $2, $3); }
  | Exp OR Exp                  { $$ = gen_tree("Exp", EXP, @$.first_line, 3, $1, $2, $3); }
  | Exp RELOP Exp               { $$ = gen_tree("Exp", EXP, @$.first_line, 3, $1, $2, $3); }
  | Exp PLUS Exp                { $$ = gen_tree("Exp", EXP, @$.first_line, 3, $1, $2, $3); }
  | Exp MINUS Exp               { $$ = gen_tree("Exp", EXP, @$.first_line, 3, $1, $2, $3); }
  | Exp STAR Exp                { $$ = gen_tree("Exp", EXP, @$.first_line, 3, $1, $2, $3); }
  | Exp DIV Exp                 { $$ = gen_tree("Exp", EXP, @$.first_line, 3, $1, $2, $3); }
  | LP Exp RP                   { $$ = gen_tree("Exp", EXP, @$.first_line, 3, $1, $2, $3); }
  | MINUS Exp %prec NEG         { $$ = gen_tree("Exp", EXP, @$.first_line, 2, $1, $2); }
  | NOT Exp                     { $$ = gen_tree("Exp", EXP, @$.first_line, 2, $1, $2); }
  | ID LP Args RP               { $$ = gen_tree("Exp", EXP, @$.first_line, 4, $1, $2, $3, $4); }
  | ID LP RP                    { $$ = gen_tree("Exp", EXP, @$.first_line, 3, $1, $2, $3); }
  | Exp LB Exp RB               { $$ = gen_tree("Exp", EXP, @$.first_line, 4, $1, $2, $3, $4); }
  | Exp DOT ID                  { $$ = gen_tree("Exp", EXP, @$.first_line, 3, $1, $2, $3); }
  | ID                          { $$ = gen_tree("Exp", EXP, @$.first_line, 1, $1); }
  | INT                         { $$ = gen_tree("Exp", EXP, @$.first_line, 1, $1); }
  | FLOAT                       { $$ = gen_tree("Exp", EXP, @$.first_line, 1, $1); }
  /* E */ | ID LP error RP              { error_end("Exp -> ID LP error RP"); }
  /* E */ | ID LP error %prec NORPRB    { error_end("Exp -> ID LP error"); }
  /* E */ | Exp LB error RB             { error_end("Exp -> Exp LB error RB"); }
  /* E */ | Exp LB error %prec NORPRB   { error_end("Exp -> Exp LB error"); }
  ;
Args : Exp COMMA Args   { $$ = gen_tree("Args", ARGS, @$.first_line, 3, $1, $2, $3); }
  | Exp                 { $$ = gen_tree("Args", ARGS, @$.first_line, 1, $1); }
  ;
%%
void yyerror(const char* msg) {
#ifdef SYNTAX_DEBUG
  /* printf("\033[1;31mError type B at Line %d: %s.\n\033[0m", yylineno, msg); */
  printf("\033[1;31mError type B at Line %d: %s.\n\033[0m", yylineno, msg);
#else
  /* printf("Error type B at Line %d: %s.\n", yylineno, msg); */
#ifdef L1_HARD
  printf("Error Type B at Line %d\n", yylineno);
#else
  printf("Error type B at Line %d: %s.\n", yylineno, msg);
#endif
#endif
  ++has_error;
}