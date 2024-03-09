grammar ifcc;

axiom : prog EOF ;

prog : 'int' 'main' '(' ')' '{' stmt* return_stmt '}' ;

stmt : var_decl_stmt
     | var_assign_stmt
     | return_stmt;

var_decl_stmt : TYPE ID ';' ;
var_assign_stmt: ID '=' (INTEGER | ID) ';' ;

return_stmt: RETURN INTEGER ';' ;

RETURN : 'return' ;
TYPE : 'int' ;

// Keywords
BREAK : 'break' ;
CASE : 'case' ;
CHAR : 'char' ;
CONTINUE : 'continue' ;
DEFAULT : 'default' ;
DO : 'do' ;
DOUBLE : 'double' ;
ELSE : 'else' ;
ENUM : 'enum' ;
EXTERN : 'extern' ;
FLOAT : 'float' ;
FOR : 'for' ;
GOTO : 'goto' ;
IF : 'if' ;
INLINE : 'inline' ;
LONG : 'long' ;
REGISTER : 'register' ;
RESTRICT : 'restrict' ;
SHORT : 'short' ;
SIGNED : 'signed' ;
SIZEOF : 'sizeof' ;
STATIC : 'static' ;
STRUCT : 'struct' ;
SWITCH : 'switch' ;
TYPEDEF : 'typedef' ;
UNION : 'union' ;
UNSIGNED : 'unsigned' ;
VOID : 'void' ;
VOLATILE : 'volatile' ;
WHILE : 'while' ;
CONST : 'const' ;

INTEGER : [0-9]+ ;
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
ID : [a-zA-Z_][a-zA-Z_0-9]* ;
