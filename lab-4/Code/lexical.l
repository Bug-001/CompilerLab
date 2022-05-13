%{
  #include "node.h"
  #define YYSTYPE struct node*
  #include "syntax.tab.h"

  extern int has_error;
  enum yytokentype repeat = -1;

  int yycolumn = 1;
  #define YY_USER_ACTION                              \
    yylloc.first_line = yylloc.last_line = yylineno;  \
    yylloc.first_column = yycolumn;                   \
    yylloc.last_column = yycolumn + yyleng - 1;       \
    yycolumn += yyleng;                               \
    if (repeat != -1) {                               \
      enum yytokentype tmp = repeat;                  \
      repeat = -1;                                    \
      return tmp;                                     \
    }

  #ifdef LEXICAL_DEBUG
    #define yield(lex)          \
      do {                      \
        printf("(%s)", #lex);   \
        return lex;             \
      } while(0)
  #else
    #define yield(lex) return lex
  #endif

  #define gen_node_and_yield(lex)                                 \
    do {                                                          \
      yylval = new_lexical_node(#lex, lex, yylineno, yytext);     \
      yield(lex);                                                 \
    } while (0)

%}

/* char set definition */
dec_digit       [0-9]
alpha_          [_a-zA-Z]

dec             0|[1-9]{dec_digit}*
oct             0[0-7]*
hex             (0x|0X)[0-9a-fA-F]*
norm_float      {dec_digit}+\.{dec_digit}+
sci_float_base  ({dec_digit}+\.{dec_digit}*)|(\.{dec_digit}+)
sci_float       {sci_float_base}[eE](\-|\+)?{dec_digit}+
id              {alpha_}({alpha_}|{dec_digit}){0,31}
comment         \/\*([^*]|(\*+[^*\/]))*\*+\/|\/\/.*
relop           ">"|"<"|">="|"<="|"=="|"!="

%option yylineno

%%
{dec}         { gen_node_and_yield(INT); }
{oct}         { gen_node_and_yield(INT); }
{hex}         { gen_node_and_yield(INT); }
{norm_float}|{sci_float} { gen_node_and_yield(FLOAT); }
  /* {norm_float}  { gen_node_and_yield(FLOAT); } */
"struct"      { gen_node_and_yield(STRUCT); }
"return"      { gen_node_and_yield(RETURN); }
"if"          { gen_node_and_yield(IF); }
"else"        { gen_node_and_yield(ELSE); }
"while"       { gen_node_and_yield(WHILE); }
"int"         { gen_node_and_yield(TYPE); }
"float"       { gen_node_and_yield(TYPE); }
{comment}     { }
{id}          { gen_node_and_yield(ID); }
";"           { gen_node_and_yield(SEMI); }
","           { gen_node_and_yield(COMMA); }
"="           { gen_node_and_yield(ASSIGNOP); }
{relop}       { gen_node_and_yield(RELOP); }
"+"           { gen_node_and_yield(PLUS); }
"-"           { gen_node_and_yield(MINUS); }
"*"           { gen_node_and_yield(STAR); }
"/"           { gen_node_and_yield(DIV); }
"&&"          { gen_node_and_yield(AND); }
"||"          { gen_node_and_yield(OR); }
"."           { gen_node_and_yield(DOT); }
"!"           { gen_node_and_yield(NOT); }
"("           { gen_node_and_yield(LP); }
")"           { gen_node_and_yield(RP); }
"["           { gen_node_and_yield(LB); }
"]"           { gen_node_and_yield(RB); }
"{"           { gen_node_and_yield(LC); }
"}"           { gen_node_and_yield(RC); }
"/*"        { ++has_error; char c; while ((c = input())); 
#ifdef L1_HARD
              printf("Error Type A at Line %d\n", yylineno);
#else
              printf("Error type A at Line %d: unterminated comment.\n", yylineno);
#endif
            }
\n          { 
  yycolumn = 1;
#ifdef LEXICAL_DEBUG
  printf("\n");
#endif
}
[ \n\t\r]   { }
.           { ++has_error;
#ifdef L1_HARD
              printf("Error Type A at Line %d\n", yylineno);
#else
              printf("Error type A at Line %d: unrecognized input `%s`.\n", yylineno, yytext);
#endif
            }
%%
