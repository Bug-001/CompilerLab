%locations
%{
  #include "lex.yy.c"
  extern void yyerror(const char* msg);
  extern int has_error;

  #ifdef SYNTAX_DEBUG
  #define error_end(err)  \
    printf("error '%s' ends at Line %d;", err, yylineno); \
    yyerrok
  #else
  #define error_end(err)  \
    yyerrok
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
%right LP RP LB RB DOT

%%
/* rules */
/* High-level Definitions */
Program : ExtDefList  { $$ = gen_tree("Program", @$.first_line, 1, $1); tree = $$; }
  ;
ExtDefList : ExtDef ExtDefList  { $$ = gen_tree("ExtDefList", @$.first_line, 2, $1, $2); }
  | /* empty */                 { $$ = gen_tree("ExtDefList", yylineno, 0); }
  ;
ExtDef : Specifier ExtDecList SEMI  { $$ = gen_tree("ExtDef", @$.first_line, 3, $1, $2, $3); }
  /* | Specifier error SEMI            { error_end("ExtDef -> Specifier error SEMI"); } */
  | Specifier SEMI                  { $$ = gen_tree("ExtDef", @$.first_line, 2, $1, $2); }
  | error SEMI                      { error_end("ExtDef -> error SEMI"); }
  | Specifier FunDec CompSt         { $$ = gen_tree("ExtDef", @$.first_line, 3, $1, $2, $3); }
  ;
ExtDecList : VarDec           { $$ = gen_tree("ExtDecList", @$.first_line, 1, $1); }
  | VarDec COMMA ExtDecList   { $$ = gen_tree("ExtDecList", @$.first_line, 3, $1, $2, $3); }
  /* | error COMMA ExtDecList    { error_end("ExtDecList -> error COMMA ExtDecList"); } */
  ;
/* Specifiers */
Specifier : TYPE      { $$ = gen_tree("Specifier", @$.first_line, 1, $1); }
  | StructSpecifier   { $$ = gen_tree("Specifier", @$.first_line, 1, $1); }
  ;
StructSpecifier : STRUCT OptTag LC DefList RC   { $$ = gen_tree("StructSpecifier", @$.first_line, 5, $1, $2, $3, $4, $5); }
  | STRUCT error LC DefList RC                  { error_end("StructSpecifier -> STRUCT error LC DefList RC"); }
  | STRUCT OptTag LC error RC                   { error_end("StructSpecifier -> STRUCT OptTag LC error RC"); }
  | STRUCT error LC error RC                    { error_end("StructSpecifier -> STRUCT error LC error RC"); }
  | STRUCT Tag                                  { $$ = gen_tree("StructSpecifier", @$.first_line, 2, $1, $2); }
  ;
OptTag : ID       { $$ = gen_tree("OptTag", @$.first_line, 1, $1); }
  | /* empty */   { $$ = gen_tree("OptTag", yylineno, 0); }
  ;
Tag : ID    { $$ = gen_tree("Tag", @$.first_line, 1, $1); }
  ;
/* Declarators */
VarDec : ID                           { $$ = gen_tree("VarDec", @$.first_line, 1, $1); }
  | VarDec LB INT RB                  { $$ = gen_tree("VarDec", @$.first_line, 4, $1, $2, $3, $4); }
  /* | error LB INT RB     { error_end("VarDec -> error LB INT RB"); } */
  | VarDec LB error RB                { error_end("VarDec -> VarDec LB error RB"); }
  | VarDec LB error %prec NORPRB      { error_end("VarDec -> VarDec LB error"); }
  /* | VarDec LB error SEMI  { error_end("VarDec -> VarDec LB error SEMI"); } */
  /* | error LB error RB   { error_end("VarDec -> error LB error RB"); } */
  ;
FunDec : ID LP VarList RP   { $$ = gen_tree("FunDec", @$.first_line, 4, $1, $2, $3, $4); }
  /* | ID LP error RP          { error_end("FunDec -> ID LP error RP"); } */
  | error LP VarList RP     { error_end("FunDec -> error LP VarList RP"); }
  /* | error LP error RP       { error_end("FunDec -> error LP error RP"); } */
  | ID LP RP                { $$ = gen_tree("FunDec", @$.first_line, 3, $1, $2, $3); }
  | ID LP error             { error_end("FunDec -> ID LP error"); }
  /* | error LP RP             { error_end("FunDec -> error LP RP"); } */
  /* | error LP error          { error_end("FunDec -> error LP error"); } */
  ;
VarList : ParamDec COMMA VarList  { $$ = gen_tree("VarList", @$.first_line, 3, $1, $2, $3); }
  | error COMMA VarList           { error_end("VarList -> error COMMA VarList"); }
  | ParamDec                      { $$ = gen_tree("VarList", @$.first_line, 1, $1); }
  ;
ParamDec : Specifier VarDec   { $$ = gen_tree("ParamDec", @$.first_line, 2, $1, $2); }
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
  | error SEMI                      { error_end("Stmt -> error SEMI"); }
  | CompSt                          { $$ = gen_tree("Stmt", @$.first_line, 1, $1); }
  | RETURN Exp SEMI                 { $$ = gen_tree("Stmt", @$.first_line, 3, $1, $2, $3); }
  | RETURN error SEMI               { error_end("Stmt -> RETURN error SEMI"); }
  | IF LP Exp RP Stmt %prec LTE     { $$ = gen_tree("Stmt", @$.first_line, 5, $1, $2, $3, $4, $5); }
  | IF LP error RP Stmt %prec LTE   { error_end("Stmt -> IF LP error RP Stmt"); }
  | IF LP error Stmt %prec LTE      { error_end("Stmt -> IF LP error Stmt"); }
  | IF LP Exp RP Stmt ELSE Stmt     { $$ = gen_tree("Stmt", @$.first_line, 7, $1, $2, $3, $4, $5, $6, $7); }
  | IF LP error RP Stmt ELSE Stmt   { error_end("Stmt -> IF LP error RP Stmt ELSE Stmt"); }
  | IF LP error Stmt ELSE Stmt      { error_end("Stmt -> IF LP Exp error Stmt ELSE Stmt"); }
  | WHILE LP Exp RP Stmt            { $$ = gen_tree("Stmt", @$.first_line, 5, $1, $2, $3, $4, $5); }
  | WHILE LP error RP Stmt          { error_end("Stmt -> WHILE LP error RP Stmt"); }
  /* | WHILE LP error Stmt             { error_end("Stmt -> WHILE LP error Stmt"); } */
  ;
/* Local Definitions */
DefList : Def DefList   { $$ = gen_tree("DefList", @$.first_line, 2, $1, $2); }
  | /* empty */         { $$ = gen_tree("DefList", yylineno, 0); }
  /* | error DefList       { error_end("DefList -> error DefList"); } */
  ;
Def : Specifier DecList SEMI  { $$ = gen_tree("Def", @$.first_line, 3, $1, $2, $3); }
  | Specifier error SEMI      { error_end("Def -> Specifier error SEMI"); }
  | error SEMI                { error_end("Def -> error SEMI"); }
  ;
DecList : Dec             { $$ = gen_tree("DecList", @$.first_line, 1, $1); }
  | Dec COMMA DecList     { $$ = gen_tree("DecList", @$.first_line, 3, $1, $2, $3); }
  /* | error COMMA DecList   { error_end("DecList -> error COMMA DecList"); } */
  ;
Dec : VarDec              { $$ = gen_tree("Dec", @$.first_line, 1, $1); }
  /* | error ASSIGNOP Exp    { error_end("Dec -> error ASSIGNOP Exp"); } */
  | VarDec ASSIGNOP Exp   { $$ = gen_tree("Dec", @$.first_line, 3, $1, $2, $3); }
  ;
/* Expressions */
Exp : Exp ASSIGNOP Exp          { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  /* | error ASSIGNOP Exp    { error_end("Exp -> error ASSIGNOP Exp"); } */
  | Exp AND Exp                 { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  /* | error AND Exp         { error_end("Exp -> error AND Exp"); } */
  | Exp OR Exp                  { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  /* | error OR Exp          { error_end("Exp -> error OR Exp"); } */
  | Exp RELOP Exp               { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  /* | error RELOP Exp       { error_end("Exp -> error RELOP Exp"); } */
  | Exp PLUS Exp                { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  /* | error PLUS Exp        { error_end("Exp -> error PLUS Exp"); } */
  | Exp MINUS Exp               { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  /* | error MINUS Exp       { error_end("Exp -> error MINUS Exp"); } */
  | Exp STAR Exp                { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  /* | error STAR Exp        { error_end("Exp -> error STAR Exp"); } */
  | Exp DIV Exp                 { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  /* | error DIV Exp         { error_end("Exp -> error DIV Exp"); } */
  | LP Exp RP                   { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  /* | LP error RP           { error_end("Exp -> LP error RP"); } */
  /* | LP error              { error_end("Exp -> LP error"); } */
  | MINUS Exp %prec NEG         { $$ = gen_tree("Exp", @$.first_line, 2, $1, $2); }
  | NOT Exp                     { $$ = gen_tree("Exp", @$.first_line, 2, $1, $2); }
  | ID LP Args RP               { $$ = gen_tree("Exp", @$.first_line, 4, $1, $2, $3, $4); }
  | ID LP error RP              { error_end("Exp -> ID LP error RP"); }
  | ID LP RP                    { $$ = gen_tree("Exp", @$.first_line, 3, $1, $2, $3); }
  | ID LP error %prec NORPRB    { error_end("Exp -> ID LP error"); }
  | Exp LB Exp RB               { $$ = gen_tree("Exp", @$.first_line, 4, $1, $2, $3, $4); }
  /* | error LB Exp RB       { error_end("Exp -> error LB Exp RB"); } */
  | Exp LB error RB             { error_end("Exp -> Exp LB error RB"); }
  | Exp LB error %prec NORPRB   { error_end("Exp -> Exp LB error"); }
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
  printf("Error type B at Line %d: %s.\n", yylineno, msg);
  ++has_error;
}