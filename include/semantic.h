/**
 * Projekt: Implementace překladače imperativního jazyka IFJ26
 *
 * Soubor: semantic.h
 * Popis: Sémantická analýza: kontrola typů, rozsahů platnosti a volání funkcí, vkládání implicitních konverzí
 *
 * Autoři:
 * Tomáš Schablický (xschabt00)
 * Azíz Hammadi ()
 */

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symtable.h"

/*
Vstupní funkce pro sémantickou analýzu, pokud vše proběhne v pořádku, vrací 0.
Jinak nevrátí nic, jelikož se v průběhu analýzy zavola exit() s příslušným kódem.
*/
int sem_analyze(ASTNode* tree, symtable_t* functions);

/*
Registruje všechny vestavěné funkce. Volá se ještě před parserem.
*/
void sem_register_builtins(symtable_t* symtable);

/*
Callback volaný parserem pro každou definici funkce. Vloží funkci do tabulky a uloží její návratový typ
a typy parametrů. Pokud funkce již existuje, ukončí program s kódem 4.
*/
void register_user_function(symtable_t* functions, ASTNode* definition);

#endif