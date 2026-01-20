%{
#include <iostream>
#include <string>
#include "util/logger.h"
#include "parser.tab.hpp"
%}

%option noyywrap
%option yylineno

%%

"//".*                  { /* Ignore line comments */ }
[ \t\n\r]+              { /* Ignore whitespace */ }

".version"              { return DIR_VERSION; }
".target"               { return DIR_TARGET; }
".address_size"         { return DIR_ADDRESS_SIZE; }
".visible"              { return DIR_VISIBLE; }
".entry"                { return DIR_ENTRY; }
".func"                 { return DIR_FUNC; }
".param"                { return DIR_PARAM; }
".reg"                  { return DIR_REG; }

[a-zA-Z_][a-zA-Z0-9_]*      { yylval.str = new std::string(yytext); return IDENTIFIER; }
\$[a-zA-Z0-9_]+             { yylval.str = new std::string(yytext); return LABEL; }
\%[a-zA-Z0-9_]+(\.[a-zA-Z0-9_]+)?    { yylval.str = new std::string(yytext); return REGISTER; }
\@[!]?\%[a-zA-Z0-9_]+       { yylval.str = new std::string(yytext); return PREDICATE; }

0[xX][0-9a-fA-F]+           { yylval.str = new std::string(yytext); return IMM_HEX; }
0[fF][0-9a-fA-F]+           { yylval.str = new std::string(yytext); return IMM_FLOAT; }
[-]?[0-9]+                  { yylval.num = std::stoll(yytext); return IMM_INT; }
[-]?[0-9]+\.[0-9]+          { yylval.fnum = std::stod(yytext); return IMM_DEC; }

"{"|"}"|"["|"]"|"("|")"     { return yytext[0]; }
"<"|">"|"+"|"-"|","|";"|":" { return yytext[0]; }
"."                         { return yytext[0]; }

.                           {
    Logger& log = Logger::getInstance();
    log.loge(1, "Lexer error at line", yylineno, ": Unkown token, matching \"", yytext, "\"");
}
%%
