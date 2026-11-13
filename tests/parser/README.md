# Testy parseru

Příklady v této složce pokrývají syntaxi IFJ26 včetně prologu, anotací,
deklarace, funkce, bloky, parametry, argumenty, odsazení, středníky,
výrazy a návraty.

Soubor .scala se předává překladači na standardní vstup. Číslo před
prvním podtržítkem v názvu souboru určuje očekávaný návratový kód.
Předpona 0_ značí syntakticky platný vstup, 2_ syntaktickou chybu.
Některé platné příklady ověřují pouze syntaxi a záměrně nemají @main.

Pokud má příklad vedle zdroje soubory .in a .out, určují standardní
vstup a očekávaný výstup přeloženého programu.
