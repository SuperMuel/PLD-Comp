grammar ifcc;

axiom : prog EOF ;

prog : 'int' 'main' '(' ')' '{' stmt* return_stmt '}' ;

stmt : var_decl_stmt
     | var_assign_stmt
     | return_stmt;

var_decl_stmt : TYPE ID ';' ;
var_assign_stmt: ID '=' (CONST | ID) ';' ;

return_stmt: RETURN CONST ';' ;

RETURN : 'return' ;
TYPE : 'int' ;
CONST : [0-9]+ ;
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
ID : [a-zA-Z_][a-zA-Z_1-9]* ;
