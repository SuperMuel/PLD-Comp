grammar ifcc;

axiom : prog EOF ;

prog : TYPE 'main' '(' ')' '{' stmt* return_stmt '}' ;

stmt : var_decl_stmt
     | array_decl_stmt
     | var_assign_stmt
     | array_assign_stmt
     | if_stmt
     | while_stmt
     | block
     | return_stmt;

var_decl_stmt : TYPE var_decl_member (',' var_decl_member)* ';';
var_decl_member: ID ('=' expr)?;
array_decl_stmt: TYPE ID '[' INTEGER_LITERAL ']' /*('=' '{' expr_list '}')?*/ ';' ;
expr_list: expr (',' expr)* ;

var_assign_stmt: ID '=' expr ';' ;
array_assign_stmt: ID '[' expr ']' '=' expr ';' ;
if_stmt: IF '(' expr ')' block #if
       | IF '(' expr ')' if_block=block ELSE else_block=block #if_else
       ;
while_stmt: WHILE '(' expr ')' block;

block: '{' stmt* '}';

expr : ID '[' expr ']' #array
     | '(' expr ')' #par
     | expr op=('*' | '/' | '%') expr #multdiv
     | expr op=('+' | '-') expr #addsub
     | expr op=('<' | '<=' | '>' | '>=') expr #cmp
     | expr op=('==' | '!=') expr #eq
     | expr '&' expr #b_and
     | expr '^' expr #b_xor
     | expr '|' expr #b_or
     | (INTEGER_LITERAL | CHAR_LITERAL | ID) #val
     ;

return_stmt: RETURN expr ';' ;

TYPE : INT | CHAR ;

// Types
INT : 'int' ;
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

INTEGER_LITERAL : [0-9]+ ;
CHAR_LITERAL : '\'' . '\'' ;

COMMENT : '/*' .*? '*/' -> skip ;
INLINE_COMMENT : '//' ~[\r\n]* -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
ID : [a-zA-Z_][a-zA-Z_0-9]* ;
