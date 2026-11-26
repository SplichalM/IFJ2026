/**
 * Projekt: Implementace překladače imperativního jazyka IFJ26
 *
 * Soubor: codegen_expr.c (semantic)
 * Popis: Hlavičkový soubor generátoru výrazů: rozhraní pro překlad výrazů na zásobníku a generování pomocných vestavěných funkci
 * Autor:
 * Azíz Hammadi (xhammaa00)
 */


#ifndef CODEGEN_EXPR_H
#define CODEGEN_EXPR_H
#include "codegen.h"
#include "ast.h"

/* Vygeneruje instrukce IFJcode26 pro vyhodnoceni vyrazu; vysledek zanecha na vrcholu datoveho zasobniku. */
void gen_expr(ASTNode* node);

/* Vypise na konec vystupniho programu pomocne vestavene funkce (napr. $builtin$subStr). */
void gen_builtin_helpers(void);

#endif