#ifndef AST_H
#define AST_H

#include <stdbool.h>

typedef enum {
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_UNIT,
    TYPE_UNKNOWN,
    TYPE_BOOL,
    TYPE_ANY 
} DataType;

typedef enum {
    LIT_INT,
    LIT_DOUBLE,
    LIT_STRING,
    LIT_UNIT
} LiteralType;

typedef enum {
    NODE_PROGRAM,           // Kořen programu 
    NODE_BLOCK,             // Blok 
    NODE_VAR_DECL,          // Deklarace proměnné 
    NODE_ASSIGN,            // Přiřazení 
    NODE_FUNC_DEF,          // Definice funkce
    NODE_FUNC_PARAM,        // Parametr funkce
    NODE_FUNC_CALL,         // Volání funkce
    NODE_IF,                // Podmínka if-then-else
    NODE_WHILE,             // Cyklus while
    NODE_RETURN,            // Návrat z funkce (return)
    NODE_BINARY_OP,         // Binární operátor (+, -, *, /, <, ==, ...)
    NODE_UNARY_OP,          // Unární operátor (-, !)
    NODE_IDENTIFIER,        // Identifikátor proměnné
    NODE_LITERAL,           // Literál (číslo, řetězec, ())
    NODE_LIST,              // Obecný seznam (argumenty, příkazy, parametry)
    NODE_INT2DOUBLE,        // Implicitní konverze z Int na Double
    NODE_MEMBER             // Přístup ke členu hodnoty, například text.length
} NodeType;

typedef struct ASTNode {
    NodeType type;
    DataType data_type;
    char *unique_name; 
    struct ASTNode *next;   
    
    union {
        struct {
            LiteralType type;
            union {
                int i_val;    
                double d_val; 
                char *s_val;
            } val;
        } literal;

        struct {
            char *name;
        } identifier;

        struct {
            struct ASTNode *receiver;
            char *name;
        } member;

        struct {
            char *name;
            DataType declared_type; 
            struct ASTNode *value;  
            bool is_mutable;        
        } var_decl;

        struct {
            char *target_name;
            struct ASTNode *value;
        } assign;

        struct {
            char *name;
            DataType param_type;
        } func_param;

        struct {
            char *name;
            struct ASTNode *params;     
            DataType return_type;       
            struct ASTNode *body;       
            bool is_main;               
        } func_def;

        struct {
            char *name;
            struct ASTNode *args;       
        } func_call;

        struct {
            struct ASTNode *cond;
            struct ASTNode *then_block;
            struct ASTNode *else_block;
        } if_stmt;

        struct {
            struct ASTNode *cond;
            struct ASTNode *body;
        } while_stmt;

        struct {
            struct ASTNode *expr;
        } ret_stmt;

        struct {
            char *op;
            struct ASTNode *left;       
            struct ASTNode *right;      
        } op;

        struct {
            struct ASTNode *head;
            struct ASTNode *tail;       
        } list;

    } as;
} ASTNode;

ASTNode *ast_create_list(NodeType type);
void ast_list_append(ASTNode *list_node, ASTNode *item);

ASTNode *ast_create_literal_int(int val);
ASTNode *ast_create_literal_double(double val);
ASTNode *ast_create_literal_string(char *val); 
ASTNode *ast_create_literal_unit(void);

ASTNode *ast_create_ident(char *name);
ASTNode *ast_create_member(ASTNode *receiver, const char *name);
ASTNode *ast_create_binary_op(char *op, ASTNode *left, ASTNode *right);
ASTNode *ast_create_unary_op(char *op, ASTNode *expr);

ASTNode *ast_create_var_decl(char *name, DataType type, ASTNode *value, bool is_mutable);
ASTNode *ast_create_assign(char *target, ASTNode *value);

ASTNode *ast_create_func_param(char *name, DataType type);
ASTNode *ast_create_func_def(char *name, ASTNode *params, DataType ret_type, ASTNode *body, bool is_main);
ASTNode *ast_create_func_call(char *name, ASTNode *args);

ASTNode *ast_create_if(ASTNode *cond, ASTNode *then_block, ASTNode *else_block);
ASTNode *ast_create_while(ASTNode *cond, ASTNode *body);
ASTNode *ast_create_return(ASTNode *expr);
ASTNode *ast_create_block(ASTNode *stmt_list);

void ast_free(ASTNode *node);

#endif 