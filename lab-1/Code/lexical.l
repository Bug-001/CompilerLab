%{
  #include "node.h"
  #define YYSTYPE struct node*
  #include "syntax.tab.h"

  extern int has_error;

  int yycolumn = 1;
  #define YY_USER_ACTION                              \
    yylloc.first_line = yylloc.last_line = yylineno;  \
    yylloc.first_column = yycolumn;                   \
    yylloc.last_column = yycolumn + yyleng - 1;       \
    yycolumn += yyleng;

  #ifdef LEXICAL_DEBUG
    #define yield(lex)          \
      do {                      \
        printf("(%s)", #lex);   \
        return lex;             \
      } while(0)
  #else
    #define yield(lex) return lex
  #endif

  #define gen_info_lexical_node_and_yield(lex, info)  \
    do {                                              \
      yylval = new_lexical_node(#lex, (info));        \
      yield(lex);                                     \
    } while (0)
    
  #define gen_lexical_node_and_yield(lex)  \
    gen_info_lexical_node_and_yield(lex, "unused")

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

%option yylineno

%%
{dec} { 
  yylval = new_lexical_node("INT", "unused");
  sscanf(yytext, "%d", &yylval->i_val);
  yield(INT);
}
{oct} {
  yylval = new_lexical_node("INT", "unused");
  sscanf(yytext, "%o", &yylval->i_val);
  yield(INT);
}
{hex} {
  yylval = new_lexical_node("INT", "unused");
  sscanf(yytext, "%x", &yylval->i_val);
  yield(INT);
}
{norm_float}|{sci_float} { 
  yylval = new_lexical_node("FLOAT", "unused");
  yylval->f_val = atof(yytext);
  yylval->info = strdup(yytext);  /* XXX: Be cautious to strdup! */
  yield(FLOAT);
}
"struct"    { gen_lexical_node_and_yield(STRUCT); }
"return"    { gen_lexical_node_and_yield(RETURN); }
"if"        { gen_lexical_node_and_yield(IF); }
"else"      { gen_lexical_node_and_yield(ELSE); }
"while"     { gen_lexical_node_and_yield(WHILE); }
"int"       { gen_info_lexical_node_and_yield(TYPE, "int"); }
"float"     { gen_info_lexical_node_and_yield(TYPE, "float"); }
{comment}   { }
{id}        { gen_info_lexical_node_and_yield(ID, strdup(yytext)); /* XXX: Be cautious to strdup! */ }
";"         { gen_lexical_node_and_yield(SEMI); }
","         { gen_lexical_node_and_yield(COMMA); }
"="         { gen_lexical_node_and_yield(ASSIGNOP); }
">"|"<"|">="|"<="|"=="|"!=" { gen_info_lexical_node_and_yield(RELOP, strdup(yytext)); /* XXX: Be cautious to strdup! */ }
"+"         { gen_lexical_node_and_yield(PLUS); }
"-"         { gen_lexical_node_and_yield(MINUS); }
"*"         { gen_lexical_node_and_yield(STAR); }
"/"         { gen_lexical_node_and_yield(DIV); }
"&&"        { gen_lexical_node_and_yield(AND); }
"||"        { gen_lexical_node_and_yield(OR); }
"."         { gen_lexical_node_and_yield(DOT); }
"!"         { gen_lexical_node_and_yield(NOT); }
"("         { gen_lexical_node_and_yield(LP); }
")"         { gen_lexical_node_and_yield(RP); }
"["         { gen_lexical_node_and_yield(LB); }
"]"         { gen_lexical_node_and_yield(RB); }
"{"         { gen_lexical_node_and_yield(LC); }
"}"         { gen_lexical_node_and_yield(RC); }
"/*"        { ++has_error; char c; while ((c = input())); 
              printf("Error type A at Line %d: unterminated comment.\n", yylineno, yytext); }
\n          { 
  yycolumn = 1;
#ifdef LEXICAL_DEBUG
  printf("\n");
#endif
}
[ \n\t\r]   { }
.           { ++has_error;
              printf("Error type A at Line %d: unrecognized input `%s`.\n", yylineno, yytext); }

%%
