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
Program : ExtDefList  { $$ = gen_tree("Program", @$.first_line, 1, $1); tree = $$; }
  ;
ExtDefList : ExtDef ExtDefList  { $$ = gen_tree("ExtDefList", @$.first_line, 2, $1, $2); }
  | /* empty */                 { $$ = gen_tree("ExtDefList", yylineno, 0); }
  ;
ExtDef : Specifier ExtDecList SEMI  { $$ = gen_tree("ExtDef", @$.first_line, 3, $1, $2, $3); }
  | Specifier SEMI                  { $$ = gen_tree("ExtDef", @$.first_line, 2, $1, $2); }
  | error SEMI                      { error_end("ExtDef -> error SEMI"); yyerrok; }
  | Specifier FunDec CompSt         { $$ = gen_tree("ExtDef", @$.first_line, 3, $1, $2, $3); }
  ;
ExtDecList : VarDec           { $$ = gen_tree("ExtDecList", @$.first_line, 1, $1); }
  | VarDec COMMA ExtDecList   { $$ = gen_tree("ExtDecList", @$.first_line, 3, $1, $2, $3); }
  ;
/* Specifiers */
Specifier : TYPE      { $$ = gen_tree("Specifier", @$.first_line, 1, $1); }
  | StructSpecifier   { $$ = gen_tree("Specifier", @$.first_line, 1, $1); }
  ;
StructSpecifier : STRUCT OptTag LC DefList RC   { $$ = gen_tree("StructSpecifier", @$.first_line, 5, $1, $2, $3, $4, $5); }
  | STRUCT error LC DefList RC                  { error_end("StructSpecifier -> STRUCT error LC DefList RC"); }
  /* | STRUCT OptTag LC error RC                   { error_end("StructSpecifier -> STRUCT OptTag LC error RC"); } */
  /* | STRUCT error LC error RC                    { error_end("StructSpecifier -> STRUCT error LC error RC"); } */
  | STRUCT Tag                                  { $$ = gen_tree("StructSpecifier", @$.first_line, 2, $1, $2); }
  ;
OptTag : ID       { $$ = gen_tree("OptTag", @$.first_line, 1, $1); }
  | /* empty */   { $$ = gen_tree("OptTag", yylineno, 0); }
  ;
Tag : ID    { $$ = gen_tree("Tag", @$.first_line, 1, $1); }
  ;
/* Declarators */
VarDec : ID                           { $$ = gen_tree("VarDec", @$.first_line, 1, $1); }
  | error COMMA                       { error_end("VarDec -> error [COMMA]"); repeat = COMMA; }
  | error SEMI                        { error_end("VarDec -> error [SEMI]"); repeat = SEMI; yyerrok; }
  | VarDec LB INT RB                  { $$ = gen_tree("VarDec", @$.first_line, 4, $1, $2, $3, $4); }
  | VarDec LB error RB                { error_end("VarDec -> VarDec LB error RB"); }
  | VarDec LB error COMMA             { error_end("VarDec -> VarDec LB error [COMMA]"); repeat = COMMA; }
  | VarDec LB error SEMI              { error_end("VarDec -> VarDec LB error [SEMI]"); repeat = SEMI; yyerrok; }
  ;
FunDec : ID LP VarList RP   { $$ = gen_tree("FunDec", @$.first_line, 4, $1, $2, $3, $4); }
  /* | ID LP error RP          { error_end("FunDec -> ID LP error RP"); } */
  | error LP VarList RP     { error_end("FunDec -> error LP VarList RP"); }
  /* | error LP error RP       { error_end("FunDec -> error LP error RP"); } */
  | ID LP RP                { $$ = gen_tree("FunDec", @$.first_line, 3, $1, $2, $3); }
  /* | ID LP error             { error_end("FunDec -> ID LP error"); } */
  | error LP RP             { error_end("FunDec -> error LP RP"); }
  | ID LP error LC          { error_end("FunDec -> ID error [LC]"); repeat = LC; }
  /* | error LP error          { error_end("FunDec -> error LP error"); } */
  /* | error LB                { error_end("FunDec -> error LB"); repeat = LB; } */
  ;
VarList : ParamDec COMMA VarList  { $$ = gen_tree("VarList", @$.first_line, 3, $1, $2, $3); }
  /* | error COMMA VarList           { error_end("VarList -> error COMMA VarList"); } */
  | ParamDec                      { $$ = gen_tree("VarList", @$.first_line, 1, $1); }
  ;
ParamDec : Specifier VarDec   { $$ = gen_tree("ParamDec", @$.first_line, 2, $1, $2); }
  | error COMMA               { error_end("ParamDec -> error [COMMA]"); repeat = COMMA;}
  /* | error RP                  { error_end("ParamDec -> error [RP]"); repeat = RP; } */
  ;
/* Statements */
CompSt : LC DefList StmtList RC   { $$ = gen_tree("CompSt", @$.first_line, 4, $1, $2, $3, $4); }
  /* | LC DefList StmtList error     { error_end("CompSt -> LC DefList StmtList error"); } */
  ;
StmtList : Stmt StmtList  { $$ = gen_tree("StmtList", @$.first_line, 2, $1, $2); }
  | /* empty */           { $$ = gen_tree("StmtList", yylineno, 0); }
  /* | error SEMI StmtList   { error_end("StmtList -> error SEMI StmtList"); } */
  /* | error StmtList   { error_end("StmtList -> error StmtList"); } */
  ;
Stmt : Exp SEMI                     { $$ = gen_tree("Stmt", @$.first_line, 2, $1, $2); }
  /* | error SEMI                      { error_end("Stmt -> error SEMI"); yyerrok; } */
  | CompSt                          { $$ = gen_tree("Stmt", @$.first_line, 1, $1); }
  | RETURN Exp SEMI                 { $$ = gen_tree("Stmt", @$.first_line, 3, $1, $2, $3); }
  | RETURN error SEMI               { error_end("Stmt -> RETURN error SEMI"); yyerrok; }
  | IF LP Exp RP Stmt %prec LTE     { $$ = gen_tree("Stmt", @$.first_line, 5, $1, $2, $3, $4, $5); }
  | IF LP error RP Stmt %prec LTE   { error_end("Stmt -> IF LP error RP Stmt"); }
  | IF LP error Stmt %prec LTE      { error_end("Stmt -> IF LP error Stmt"); }
  | IF LP Exp RP Stmt ELSE Stmt     { $$ = gen_tree("Stmt", @$.first_line, 7, $1, $2, $3, $4, $5, $6, $7); }
  | IF LP error RP Stmt ELSE Stmt   { error_end("Stmt -> IF LP error RP Stmt ELSE Stmt"); }
  | IF LP error Stmt ELSE Stmt      { error_end("Stmt -> IF LP Exp error Stmt ELSE Stmt"); }
  | WHILE LP Exp RP Stmt            { $$ = gen_tree("Stmt", @$.first_line, 5, $1, $2, $3, $4, $5); }
  | WHILE LP error RP Stmt          { error_end("Stmt -> WHILE LP error RP Stmt"); }
  | WHILE LP error Stmt             { error_end("Stmt -> WHILE LP error Stmt"); }
  ;
/* Local Definitions */
DefList : Def DefList   { $$ = gen_tree("DefList", @$.first_line, 2, $1, $2); }
  | /* empty */         { $$ = gen_tree("DefList", yylineno, 0); }
  /* | error DefList       { error_end("DefList -> error DefList"); } */
  ;
Def : Specifier DecList SEMI  { $$ = gen_tree("Def", @$.first_line, 3, $1, $2, $3); }
  | Specifier error SEMI      { error_end("Def -> Specifier error SEMI"); yyerrok; }
  /* | error DecList SEMI        { error_end("Def -> error DecList SEMI"); } */
  | error SEMI                { error_end("Def -> error SEMI"); yyerrok; }
  ;
DecList : Dec             { $$ = gen_tree("DecList", @$.first_line, 1, $1); }
  | Dec COMMA DecList     { $$ = gen_tree("DecList", @$.first_line, 3, $1, $2, $3); }
  /* | error COMMA DecList   { error_end("DecList -> error COMMA DecList"); } */
  ;
Dec : VarDec              { $$ = gen_tree("Dec", @$.first_line, 1, $1); }
  /* | error ASSIGNOP Exp    { error_end("Dec -> error ASSIGNOP Exp"); } */
  | VarDec ASSIGNOP Exp   { $$ = gen_tree("Dec", @$.first_line, 3, $1, $2, $3); }
  | VarDec ASSIGNOP COMMA { yyerror("Expected expression"); repeat = COMMA; YYERROR; }
  | VarDec ASSIGNOP SEMI  { yyerror("Expected expression"); repeat = SEMI; YYERROR; }
  ;
/* Expressions */
Exp : Exp ASSIGNOP Exp          { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  | Exp AND Exp                 { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  | Exp OR Exp                  { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  | Exp RELOP Exp               { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  | Exp PLUS Exp                { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  | Exp MINUS Exp               { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  | Exp STAR Exp                { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  | Exp DIV Exp                 { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  | LP Exp RP                   { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  | MINUS Exp %prec NEG         { $$ = gen_tree("Exp", @$.first_line, 2, $1, $2); }
  | NOT Exp                     { $$ = gen_tree("Exp", @$.first_line, 2, $1, $2); }
  | ID LP Args RP               { $$ = gen_tree("Exp", @$.first_line, 4, $1, $2, $3, $4); }
  | ID LP error RP              { error_end("Exp -> ID LP error RP"); }
  | ID LP error %prec NORPRB    { error_end("Exp -> ID LP error"); }
  | ID LP RP                    { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  /* | ID LP error SEMI            { error_end("Exp -> ID LP error [SEMI]"); repeat = SEMI; } */
  | Exp LB Exp RB               { $$ = gen_tree("Exp", @$.first_line, 4, $1, $2, $3, $4); }
  | Exp LB error RB             { error_end("Exp -> Exp LB error RB"); }
  | Exp LB error %prec NORPRB   { error_end("Exp -> Exp LB error"); }
  /* | error LB Exp RB       { error_end("Exp -> error LB Exp RB"); } */
  /* | Exp LB error RB             { error_end("Exp -> Exp LB error RB"); } */
  /* | Exp LB error %prec NORPRB   { error_end("Exp -> Exp LB error"); } */
  /* | error LB error RB     { error_end("Exp -> error LB error RB"); } */
  | Exp DOT ID                  { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  /* | error DOT ID          { error_end("Exp -> error DOT ID"); } */
  | ID                          { $$ = gen_tree("Exp", @$.first_line, 1, $1); }
  | INT                         { $$ = gen_tree("Exp", @$.first_line, 1, $1); }
  | FLOAT                       { $$ = gen_tree("Exp", @$.first_line, 1, $1); }
  ;
Args : Exp COMMA Args   { $$ = gen_tree("Args", @$.first_line, 3, $1, $2, $3); }
  /* | error COMMA Args    { error_end("Args -> error COMMA Args"); } */
  | Exp                 { $$ = gen_tree("Args", @$.first_line, 1, $1); }
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