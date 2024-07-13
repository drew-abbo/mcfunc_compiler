# Style Guide

The style of C++ code in this repo is based on the
[LLVM style](https://llvm.org/docs/CodingStandards.html). There are changes from
this standard which are labeled in this document.

## The Basics

- Lines have an 100 character width limit (limit is 80 for comments).
- Use 2 spaces for indentation, never tabs.
- `.h` and `.cpp` should be used for headers and implementation files.
- Prefer post-increment (`i++` over `++i`) (different from LLVM).
- Prefer `//` over `/*` `*/`.
- Use `const` as often as possible *but* do not use `const` for function
  parameters that are passed in by value (still use when passing by reference).
- Don't use `noexcept`, just document if a function throws with `/// \throws`
  and avoid exceptions wherever reasonable.

## Naming

- Type names should be `PascalCase`.
- Variable names should be `cascalCase` (different from LLVM).
- Function names should be `camelCase`.
- Namespace names should be `snake_case`.
- Macro names should be `UPPER_SNAKE_CASE`.
- Enum values should be `UPPER_SNAKE_CASE` (different from LLVM).

File names should match the thing they contain. For example a file `foo.h` may
contain the namespace `foo`, while `Bar.cpp` may implement the class `Bar`. If
it's ambiguous (this file for example) prefer `snake_case`.

Enums that represent a "kind"/"type" of something should have the `Kind` suffix
(e.g. `TokenKind`). Enum members should either have a prefix (e.g. `TK_`) or be
an enum class.

```cpp
// good enum example 1 (uses prefix)
enum TokenKind {
  TK_L_PAREN,
  TK_R_PAREN,
  // ...
};

// good enum example 2 (enum class)
enum class TokenKind {
  L_PAREN,
  R_PAREN,
  // ...
};
```

Use `std` naming conventions for something that mirrors a standard library
feature (like `push_back()`).

## Attach Pointer/Reference Symbols on the Left

This makes it appear more like a "pointer type". Don't declare multiple
pointer/reference variables in the same declaration.

```cpp
int* xPtr = &x;
```

This is different from the LLVM standard.

## Code Block Format

Don't use braces for blocks of code that are 1 line.

```cpp
if (condition)
  foo();
```

Put a space before the parenthesis for `for`, `while`, and `if`, but not for
function/macro calls.

```cpp
if (foo()) // ...
```

## Using Namespaces

Don't indent namespaces and put the namespace name at the end of the namespace.

```cpp
namespace foo {

int bar(char* s) {
  // ...
}

} // namespace foo
```

Follow this pattern for a namespace's implementation file:

```cpp
// Foo.h
namespace foo {

int bar(const char* s);

}
```

```cpp
// Foo.cpp
#include "Foo.h"

using namespace foo;

int foo::bar(const char* s) {
  // ...
}
```

Never use `using namespace XXX;` unless writing the implementation for a
namespace (e.g. never do `using namespace std;`).

Use anonymous namespaces only for classes that should be static to a file, also
try and make them as small as possible. Use `static` for variables and
functions.

```cpp
namespace {
class Foo {
  // ...
};
} // namespace
```

## Use Asserts

Asserts get removed when compiling a release version so include `<cassert>` and
use them all over the place since it helps to ensure assumptions are valid.

```cpp
assert(x > 0 && "x was not more than 0");
```

## Internal Headers Should be Kept Private

Internal headers should be kept private. This means that classes/namespaces with
multiple implementation files should have 1 main header in `include/`.

[src](https://llvm.org/docs/CodingStandards.html#keep-internal-headers-private)

## Switch Case With Enumerations

Do not use the `default` label if the switch case covers all cases because
`-Wswitch` won't fire if a new case is added.

## Don't Use `inline`

Don't use the `inline` keyword. Compilers are smarter than you are in this
regard.

## Doc Comments

All files, classes, and methods in "external" headers (anything in `include/`)
should have a doc comment. Here are some examples:

```cpp
/// Does foo and bar.
///
/// Does not do foo the usual way if \p baz is true.
///
/// Typical usage:
/// \code
///   fooBar(false, "hi", res);
/// \endcode
///
/// \param str kind of foo to do.
/// \param [out] result filled with bar sequence on foo success.
///
/// \returns true on success.
bool fooBar(bool baz, std::string &str, const std::vector<int>& result);
```

```cpp
/// \file Contains ...

/// Does ...
void foo();
```
