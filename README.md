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
- Partial support for `break` and `continue`; see the limitations below.

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

`find` returns the zero-based position of the first match, or `-1` if no match exists. `split()` splits on whitespace; `split(separator)` uses the specified separator string and may preserve empty fragments. Empty separators are not yet handled correctly.

`pop` removes and returns an element. `remove` deletes a key if it exists, but does not yet provide a return value usable in expressions.

## Value semantics

- List and string indexing starts at zero; negative indices are rejected.
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
- Loops propagate `break`, which can also stop enclosing loops. The true branch of an `if` does not propagate `continue`.
- Collection printing, comparison, and copying do not detect cycles. Avoid collections that directly or indirectly contain themselves; `shared_ptr` cycles can also prevent memory from being released.
- `split("")` does not advance through the string and can hang the program.
- Input through `hear` does not fully distinguish empty input from a failed read.
- Comments and escape sequences are not implemented; tab indentation and whitespace-only lines are not fully handled.
- There is no automated test suite yet. The examples above describe expected results and do not establish correctness for all execution cases.

Priorities: error and loop-control propagation, type validation, regression tests, local function environments, and splitting the implementation into modules.
