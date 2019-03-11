.. BrcLab User Guide documentation master file, created by
   sphinx-quickstart on Mon Mar  4 20:12:07 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Contents:

.. toctree::
   :maxdepth: 2


JN50 View User Guide!
=====================

Význam signalizačních polí
--------------------------

============== ===========================
Barva pole     Význam
============== ===========================
Šedivá         stav není znám
Bílá           stav není aktivní
Zelená         nastal popsaný aktivní stav
Červená        nastal popsaný chybový stav 
============== ===========================

OCL
  overhead contact line (trolej)

Communication
  stav připojení zařízení přes GSM a server s aplikací JN50View

Run
  zeleně zapnutí měniče; červeně vypnutý měnič; šedivě pokud stav není znám

Service run
   červeně zařízení je v servisním modu a nelze dálkově spravovat

Standby
   zeleně měnič je pod napětím; sekundární střídavá strana je bez napětí

OCL Voltage
   signalizuje zeleně pokud je měnič pod napětím

Error HW
    na zařízení se objevila hardwarová chyba

Error SW
  na zařízení se objevila jedna z SW chyb (porucha TZ, porucha maskovania TZ, ERUI porucha)

Blocked
 pulzy IGBT tranzistorů jsou zablokovány

Door open
 Víko měniče je otevřené

Overvoltage
 Na troleji se objevilo vyšší napětí než dovolené, přepětí > 950V

Over current
  zařízení je přetíženo na vstupní straně

Overload
  zařízení je přetíženo na výstupní straně

Run can
 spojení komunikační jednotky s měničem je navázáno

Význam multimetru
-----------------

Ampérmetr reaguje na změnu větší než ±2A, jinak se změna na displej neprojeví.

Voltmetr reaguje na změnu větší než ±10V, jinak se změna na displeji neprojeví.

Výkony jsou zobrazeny v kilowattech, citlivost po jednotkách kW.

Energie je zobrazena v kWh, citlivost po jednotkách kWh.

===================== =================
OCL Voltage           napětí na troleji
Input power           vstupní příkon zařízení
Output energy         výstupní energie zařízení
Active power          činný výkon zařízení
Apparent power        zdánlivý výkon
A                     ampérmetr (vstup, fáze)
Hz                    měřič frekvence v síti
V                     napětí (na vstupu měniče, fáze)
===================== =================

Ostatní popis
-------------

===================== =============================
DC converter          vstupní střídač
AC inverter           výstupní 3f střídač
===================== =============================


