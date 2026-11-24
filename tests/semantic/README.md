# Testy sémantiky základního IFJ26

Překladač se sémantikou se sestavuje příkazem make.
Tento cíl vyžaduje funkční sémantický modul a dostupnou implementaci
funkce register_user_function.

Soubory KOD_popis.scala v této složce se předávají překladači na standardní vstup.
Číslo před podtržítkem určuje očekávaný návratový kód podle § 6 specifikace.
Kladné případy ověřují například dopředné volání, rekurzi, odvození typů,
rozsahy platnosti a konverzi Int na Double. Záporné případy ověřují chyby
3 až 7 u jmen, redefinic, argumentů, typů a zápisu do neměnné hodnoty.
