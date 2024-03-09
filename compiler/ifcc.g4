grammar ifcc;

axiom : prog EOF ;

prog : 'int' 'main' '(' ')' '{' stmt* return_stmt '}' ;

stmt : var_decl_stmt
     | var_assign_stmt
     | return_stmt;

var_decl_stmt : TYPE ID ';' ;
var_assign_stmt: ID '=' (INTEGER | ID) ';' ;

return_stmt: RETURN INTEGER ';' ;

TYPE : 'int' ;

// Types
CHAR : 'char' ;
DOUBLE : 'double' ;
LONG : 'long' ;
VOID : 'void' ;
SHORT : 'short' ;
FLOAT : 'float' ;


// Keywords
RETURN : 'return' ;
BREAK : 'break' ;
CASE : 'case' ;
CONTINUE : 'continue' ;
DEFAULT : 'default' ;
DO : 'do' ;
ELSE : 'else' ;
ENUM : 'enum' ;
EXTERN : 'extern' ;
FOR : 'for' ;
GOTO : 'goto' ;
IF : 'if' ;
INLINE : 'inline' ;
REGISTER : 'register' ;
RESTRICT : 'restrict' ;
SIGNED : 'signed' ;
SIZEOF : 'sizeof' ;
STATIC : 'static' ;
STRUCT : 'struct' ;
SWITCH : 'switch' ;
TYPEDEF : 'typedef' ;
UNION : 'union' ;
UNSIGNED : 'unsigned' ;
VOLATILE : 'volatile' ;
WHILE : 'while' ;
CONST : 'const' ;

INTEGER : [0-9]+ ;
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
ID : [a-zA-Z_][a-zA-Z_0-9]* ;
