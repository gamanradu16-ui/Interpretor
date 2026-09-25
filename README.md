# Interpretor

Un interpretor pentru un limbaj de programare propriu, construit de la zero in C++ ca proiect educational.

Proiectul implementeaza tokenizare, parsare, un arbore AST si evaluarea programelor.

## Functionalitati

- valori `int`, `bool`, `string` si liste;
- declararea si reasignarea variabilelor;
- expresii aritmetice, logice si comparatii;
- precedenta operatorilor si paranteze;
- instructiuni `if` / `else` si `while`;
- liste, indexare si metodele `size`, `push` si `pop`;
- functii cu parametri si `return`;
- afisare prin instructiunea `shout`.

## Compilare

Este necesar un compilator cu suport pentru C++17.

```powershell
g++ -std=c++17 interpreter.cpp -o interpreter.exe
```

## Rulare

Scrie programul in `code.txt`, apoi ruleaza interpretorul din directorul proiectului:

```powershell
.\interpreter.exe
```

Rezultatele instructiunilor `shout` sunt scrise in `output.txt`.

## Exemplu

```text
fun add(a, b)
    return a + b
end

let x = 10
let y = 20
shout add(x, y)
```

Rezultat:

```text
30
```

## Sintaxa principala

```text
let x = 10
x = x + 1
shout x

if x > 5
    shout "mare"
else
    shout "mic"
end

while x < 20
    x = x + 1
end
```

Liste:

```text
let values = [10, 20, 30]
shout values[0]
values.push(40)
shout values.size()
shout values.pop()
```

## Structura proiectului

- `interpreter.cpp` - tokenizerul, parserul, evaluatorul si punctul de intrare;
- `clase.h` - tipurile de token-uri, valorile si clasele AST;
- `code.txt` - programul executat de interpretor;
- `output.txt` - fisier generat la rulare.

## Stadiu

Proiectul este in dezvoltare. Obiectivul sau este invatarea modului in care functioneaza un interpretor, nu utilizarea in productie.

