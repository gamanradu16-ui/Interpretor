# Interpretor

An interpreter for a custom programming language, implemented in C++17 as an educational project. Programs are tokenized, parsed into an abstract syntax tree (AST), and evaluated directly by walking the tree.

The project is under development. Implemented features and known limitations are documented below; it is not intended for running untrusted programs or for production use.

## Features

- Arbitrary-precision integers using Boost.Multiprecision `cpp_int`.
- Booleans, strings, lists, and dictionaries with string keys.
- `let` declarations, reassignment, and indexed assignment for lists and dictionaries.
- Arithmetic expressions, comparisons, logical operators `and`, `or`, `!`, and parentheses.
- `if` / `else`, `while`, and `for`.
- Functions with parameters and `return`.
- Input through `hear` and `hear int`; output through `shout`.
- Content-based comparison and deep copying for collections without cycles.
- `break` and `continue` in loops, including inside `if` branches.
- Full-line comments with `#` as the first character of the line.

## Requirements and build

A C++17 compiler and the Boost headers are required. The `cpp_int` backend does not require linking a separate Boost binary library.

From the project directory:

```powershell
g++ -std=c++17 -Wall -Wextra interpreter.cpp -o interpreter.exe
```

On Windows with MSYS2 UCRT64, install Boost from the MSYS2 UCRT64 terminal:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-boost
```

Use a compiler that matches your Boost installation. If GCC is installed in `C:\msys64\ucrt64\bin`, that directory must be available through `PATH`, including to the compiler's child processes. In PowerShell, set it for the current session:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
g++ -std=c++17 -Wall -Wextra interpreter.cpp -o interpreter.exe
```

## Running a program

Write your program in `code.txt`, then run the interpreter from the directory containing that file:

```powershell
.\interpreter.exe
```

`shout` writes to `output.txt`, which is recreated at startup. It does not append a newline automatically: use `endl`. `hear` reads a line from the console, while `hear int` expects an integer. Diagnostics and debugging information may appear in the console.

The input filename is fixed as `code.txt`; a custom path cannot yet be passed as a command-line argument. The reported execution time includes program processing and any time spent waiting for input.

## Syntax examples

### Functions and numbers

```text
fun add(a, b)
    return a + b
end

let result = add(10, 20)
shout result + endl
```

Expected output in `output.txt`: `30`, followed by a newline.

### Loops and lists

```text
let values = []
for let i = 0, i < 4, i = i + 1
    values.push(i * i)
end
values[2] = 99
shout values + endl
```

Expected output: `[0, 1, 99, 9]`.

### Dictionaries

```text
let person = {"name": "Ana", "age": 20}
person["age"] = 21
shout person["name"] + endl
shout person.count("age") + endl
```

`count` returns a boolean indicating whether the key exists. The display order of dictionary entries is not guaranteed.

### Strings

```text
let words = "one two three".split()
shout words.join("-") + endl
shout "one two three".find("three") + endl
```

Expected output: `one-two-three` and `8`, on separate lines.

## Available methods

| Type | Methods and behavior |
| --- | --- |
| List | `size()`, `push(value)`, `pop()`, `pop(index)`, `copy()`, `to_string()` |
| List of strings | `join()` concatenates the strings; `join(separator)` inserts a separator between elements |
| String | `size()`, `list()` to produce a list of single-byte strings, `find(text)`, `split()`, `split(separator)` |
| Dictionary | `size()`, `count(key)`, `keys()`, `values()`, `remove(key)`, `copy()`, `to_string()` |
| Integer / boolean | `to_string()` |

`find` returns the zero-based position of the first match, or `-1` if no match exists. `split()` splits on whitespace. `split("")` also splits on whitespace. With a nonempty separator, `split(separator)` uses the specified separator string and may preserve empty fragments.

`pop` removes and returns an element. `remove` deletes a key if it exists, but does not yet provide a return value usable in expressions.

## Value semantics

- List and string indexing starts at zero; negative indices are rejected.
- `break` exits the innermost executing loop. `continue` skips the remaining statements in the current iteration; a `for` loop still executes its step before checking its condition again. Both branches of `if` propagate these signals to the enclosing loop.
- Strings and numbers are copied by value. Lists and dictionaries are shared through `shared_ptr`: assigning a collection to another variable does not automatically create an independent copy.
- `copy()` recursively creates new collections for structures without cycles.
- `values()` creates a new list, but any collections contained in its values remain shared.
- Reading a missing dictionary key through indexing currently inserts that key with the numeric value zero. `count()` checks for existence without inserting anything.
- Function calls currently save and restore the variable map; separate local environments and lexical closures are not yet implemented.
- String operations work on `std::string` bytes, rather than complete Unicode characters.

## Project structure

- `clase.h`: value and token types, and AST nodes.
- `interpreter.cpp`: tokenizer, parser, evaluator, methods, and entry point.
- `code.txt`: input program.
- `output.txt`: output generated during execution.
- `.vscode`: local build and debugging configuration.

## Known limitations and next steps

- Runtime errors are not consistently distinguished from normal statement completion; some errors allow execution to continue.
- Some invalid type combinations can lead to incorrect extraction from `std::variant`; equality between different types is not handled consistently.
- Cycle handling is incomplete. List printing tracks list addresses and detects cycles reached through list elements, but cycles involving dictionaries can still escape detection. An early return can leave an address in the global tracking set and produce incomplete output or affect later printing, including `to_string()`.
- Copying detects direct list self-reference only and returns the boolean `false` on that error; an enclosing copy can treat it as an ordinary value. Indirect cycles and dictionary cycles remain unsupported. Collection equality has no cycle detection.
- Avoid collections that directly or indirectly contain themselves. Cyclic `shared_ptr` ownership can prevent memory from being released even when printing detects a cycle.
- Input through `hear` does not fully distinguish empty input from a failed read.
- Only full-line comments beginning with `#` in the first column are skipped. Indented and inline comments are not supported. Escape sequences are not implemented; tab indentation and whitespace-only lines are not fully handled.
- There is no automated test suite yet. The examples above describe expected results and do not establish correctness for all execution cases.

Priorities: consistent error propagation, complete cycle handling, type validation, regression tests, local function environments, and splitting the implementation into modules.
