grammar ifcc;

axiom : prog EOF ;

prog : func+;

func : TYPE ID '(' (TYPE ID (',' TYPE ID)*)? ')' block ;

stmt : block
     | var_decl_stmt
     | var_assign_stmt
     | if_stmt
     | while_stmt
     | return_stmt
     | function_call_stmt;

var_decl_stmt : TYPE ID ';' ;
var_assign_stmt: ID '=' expr ';' ;
if_stmt: IF '(' expr ')' block #if
       | IF '(' expr ')' if_block=block ELSE else_block=block #if_else
       ;
while_stmt: WHILE '(' expr ')' block;

block: '{' stmt* '}';
function_call_stmt : function_call ';' ;
function_call : ID '(' (expr (',' expr)*)? ')' ;

expr : '(' expr ')'             #par
     | expr op=('*' | '/') expr #multdiv
     | expr op=('+' | '-') expr #addsub
     | expr op=('<' | '<=' | '>' | '>=') expr #cmp
     | expr op=('==' | '!=') expr #eq
     | (INTEGER_LITERAL | ID)   #val
     | function_call            #function_call_expr
     | expr '&' expr #b_and
     | expr '^' expr #b_xor
     | expr '|' expr #b_or
     | (INTEGER_LITERAL | ID) #val
     ;

return_stmt: RETURN expr ';' ;

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

INTEGER_LITERAL : [0-9]+ ;

COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
ID : [a-zA-Z_][a-zA-Z_0-9]* ;
