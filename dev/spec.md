# Spec.

This document outlines a lot of the more technical aspects of how MCFunc works
as a language and as a compiler (a "specification" of how things should work).
It also outlines planned features.

- [Stages of Compilation](#stages-of-compilation)
  - [Tokenization](#tokenization)
  - [Syntax Analysis and Symbol Table Generation](#syntax-analysis-and-symbol-table-generation)
  - [Semantic Analysis](#semantic-analysis)
  - [Linking](#linking)
  - [Optimization](#optimization)
  - [Translation](#translation)
  - [Code Generation](#code-generation)
- [Advanced User Documentation](#advanced-user-documentation)
  - [Function File Naming Patterns](#function-file-naming-patterns)
  - [Tick and Load Functions](#tick-and-load-functions)
  - [File Write Restrictions](#file-write-restrictions)
  - [Importing](#importing)
- [Future Additions](#future-additions)
  - [Future Language Additions](#future-language-additions)
    - [Modules and Aliases](#modules-and-aliases)
    - [Scheduling Code](#scheduling-code)
    - [Snippets](#snippets)
    - [Control Flow](#control-flow)
    - [Selectors](#selectors)
    - [Integer Data Type](#integer-data-type)
    - [NBT Variables](#nbt-variables)
    - [Macros](#macros)
    - [Undesigned Language Additions](#undesigned-language-additions)
  - [Future Compiler Additions](#future-compiler-additions)
    - [Serialization](#serialization)

---

## Stages of Compilation

### Tokenization

All input files are read through and convered into tokens (small syntax elements
like a semicolon `;`).

### Syntax Analysis and Symbol Table Generation

Each file is individually evaluated and its syntax is checked for validity.
During this process code is grouped into more broad "statements" like
"write file" or "call function". What each file declares, defines, and uses is
saved in a collection of symbol tables.

### Semantic Analysis

Next we validate that all symbols used are declared. This includes checking the
current file's symbol table and checking the symbol tables for imported files.
Because the symbol tables for all files already exist, this can be done in
parallel.

The number of times a function is called is also tracked. This can help
optimiation remove functions that may end up unused after inlining. Exposed
functions are gived an automatic `+1` to their use counter so that they cannot
be removed (they can still be inlined).

Note that functions called do not need to be *defined* to be used, only
declared. The following will work even though there's no importing between
`foo.mcfunc` and `foo_implementation.mcfunc`:

```mcfunc
// foo.mcfunc

public void foo();
```

```mcfunc
// foo_implementation.mcfunc

public void foo() {
  /say foo;
}
```

```mcfunc
// main.mcfunc

expose "example";

import "foo.mcfunc";

void bar() {
  foo();
}
```

### Linking

All symbol tables are merged into 1 large one. Any conflicts or missing
definitions at this point will cause compilation to fail.

### Optimization

This step is optional.

With everything defined, a lot of major optimizations (like inlining) can occur.
This stage can be enabled or disabled. Other minor optimizations may occur in
previous steps regardless.

### Translation

Translation is done to convert every operation into a file write operation. This
includes things like giving functions addresses and converting all non-command
operations (like function calls) into commands. The output of this step should
be a list of file paths with contents that need to be generated.

### Code Generation

The data pack can now be generated through a series of file write operations.

---

## Advanced User Documentation

### Function File Naming Patterns

Any `.mcfunction` files created that don't represent exposed functions will
follow the following naming pattern where `$HIDDEN_NAMESPACE` is the exposed
namespace with `zzz__.` in front of it and `$SYMBOL_ID` is an arbitrary id used
for identification.

```
$HIDDEN_NAMESPACE:$SYMBOL_ID
```

Exposed functions instead follow the following naming pattern where `$NAMESPACE`
is the exposed namespace and `$EXPOSED_NAME` is the text in quotes after the
`expose` keyword in the function definition.

```
$NAMESPACE:$EXPOSED_NAME
```

So the following block of code could output the functions `foo:my_func` and
`zzz__.foo:f_1`:

```mcfunc
expose "foo";

void bar() expose "my_func" {
  /say hello;
};

void baz() {
  /say world;
};
```

This is done because it pushes functions the user should be running to the
top of the autofill list that appears when you type `/function` in-game.
Additionally, since the name would be made up of arbitrary numbers and letters
the chance that a user runs an internal function that could break something is
reduced.

### Tick and Load Functions

When a function is labeled with the `tick`/`load` keyword its symbol name is
saved in a list. After compilation is finished and the function has an address
it will be *added* to any existing `minecraft/tags/function/tick.json` or
`minecraft/tags/function/load.json` files. This means these files will be opened
up, read, functions from the namespace that's currently being compiled will be
removed, and the new functions will be added. This is done so that you can
safely compile multiple namespaces into the same data pack separately. Anything
in the `minecraft` namespace is a shared resource between all namespaces.

### File Write Restrictions

File paths must be Unix-style and use `/` (not the Windows style `\`). This
keeps it consistent with the game and with the way strings work in MCFunc (`\`
is the escape character).

You can only write files into the namespace directory of your data pack. You
can't use exact file paths or make use of `..`.

This means that none of the following file write operations are allowed:

```mcfunc
// '\' is an escape character
file "foo\bar.json" = ``;

// can't write files there
file "../foo" = ``;
file "/bin" = ``;
file "C:/Windows/System32" = ``;
```

File paths can only contain letters, numbers, underscores, dots, and dashes
(plus the slash character as a separator).

At the start of compilation the contents of the namespace folder and the hidden
namespace folder are removed. As compilation goes on files and directories will
be created. If there is any conflict here compilation will fail. This means that
the following will not work:

```mcfunc
void foo() expose "foo.mcfunction";
file "function/foo.mcfunction" = ``;
```

### Importing

Imported file paths can only contain letters, numbers, underscores, dots, and
dashes (plus the slash character as a separator). You cant use `..` and the path
must be relative.

---

## Future Additions

### Future Language Additions

#### Modules and Aliases

```mcfunc
// A module is a way to group code so that there isn't name conflicts with
// global members.

// Modules can only be imported if they are declared with the 'public' keyword.
public module my_mod {

public void foo() {
  /say foo;
}

public void baz() {
  /say baz;
}

} // my_mod

void bar() {
  // You can use public members of a module with the '::' operator.
  my_mod::foo();
}

// You can add declarations to modules later (even in different files).
public module my_mod {

// This is a different 'bar()' function from the one above.
public void bar();

} // my_mod

// You can define public module members outside of the module they are from.
void my_mod::bar() {
  /say bar 2;
}

// You can use the 'using' keyword to pull a member out of a namespace.
using my_mod::baz;

// You can do the same thing with an entire module.
using module my_mod;

void blah() {
  baz();  // This actually calls 'my_mod::baz'.
}

// You can also use the 'using' keyword to create aliases for anything.
using cool_baz = my_mod::baz;

// Modules are a requirement before any kind of library (including the standard
// library) can be created.
```

#### Scheduling Code

```mcfunc
void foo() {
  // Schedule functions.
  in 1 bar();

  // Schedule a scope or individual command.
  in 1 {
    /say hello;
  }

  // Replace the function/scope/command if it's already scheduled (appends by
  // default).
  in 1 replace
    /say goodbye;
}
```

#### Snippets

```mcfunc
void foo() {
  // Define a snippet (like a compile time macro) (definition is scoped).
  snippet MSG = `hello`;

  // Snippets can be inserted into a command, other snippet definitions, or
  // quotes.
  /tellraw @a \${MSG};

  // The 'NAMESPACE' and 'HIDDEN_NAMESPACE' snippets are hard coded (set by
  // the 'expose' keyword at the top of the file).
  /tellraw @a "Thanks for installing ${NAMESPACE}";

  // Use '&' to get the address of something (like 'foo:bar' for a function).
  /function ${&my_function};
}
```

#### Control Flow

```mcfunc
void foo() {
  // If else statements with '&&', '||', and '!' operators.
  // Individual conditions go in '``', the whole test must be in parenthesis.
  if (`entity @s[type=zombie]`)
    /tellraw @p "This is a zombie!";
  else if (`entity @s[type=husk]` || `entity @s[type=drowned]`)
    /tellraw @p "This is a type of zombie!";
  else if (!`entity @s[type=player]`)
    /tellraw @p "Cannot convert this entity to a zombie.";
  else {
    /tellraw @p "Converting to a zombie!";
    /execute at @s run summon zombie ~ ~ ~;
    /kill @s;
  }

  if (`score @s game_score matches ..0`) {
    // Early return!
    // stops executing 'foo()', regardless of how nested we are
    return;
  }

  // While loops!
  /execute at @s run: while (!`block ~ ~ ~ #minecraft:air`)
    /tp @s ~ ~1 ~;

  // 'true' and 'false' keywords are valid here.
  while (true) {
    if (`score @s game_score matches ..0`) {
      // break out of the innermost loop
      break;
    }
    else {
      /scoreboard players remove @s game_score 1;
      // return to beginning of innermost loop
      continue;
    }
  }

}
```

#### Selectors

```mcfunc
void foo() {
  // Selectors are a shorthand for tagging a single entity and using that entity
  // for a period of time. In this example, the nearest creeper is tagged and
  // then can be identified by this selector while in this scope, regardless of
  // whether it stops being the nearest at any point.
  selector NearestCreeper = `@n[type=creeper]`;

  // Selectors can be used in snippets and commands with the '@' symbol.
  // Selector names must be at least 2 characters long so as to not conflict
  // with the vanilla syntax.
  /effect give @NearestCreeper speed 10 0 true;

  // You can still use the '[]' syntax with selectors.
  /effect give @NearestCreeper[tag=extra_speed] speed 30 2 true;

  // Selected entities are "unselected" at the end of a scope (the tag is
  // removed). To prevent this you can create a selector in the global scope.
}
```

#### Integer Data Type

```mcfunc
void foo() {
  // Create an int variable.
  int x;
  // Create an assign an int variable.
  int y = 10;

  // Assign/reassign a variable with expressions with '+', '-', '*', '/', '%',
  // '==', '!=', '>', '>=', '<', '<=', '&&', '||', '!', '=', '+=', '-=', '*=',
  // '/=', and '%=', '++', and '--', operators with precedence (same as C) plus
  // ability to call functions.
  x = 10 + (y * 5);

  // Declare a constant (cannot change).
  const int z = 100;

  // Declare an exposed variable with 'is'.
  int player_score is "@s game_score";

  // 'bool' is an alias of 'int'.
  bool score_is_good = true;

  // Conditions with expressions (also works with while loops).
  if (player_score < 100) {
    score_is_good = false;
    if (player_score < 0)
      player_score = 0;
  }

  // Combine both kinds of if statements
  if (score_is_good && `loaded ~ ~ ~`)
    /summon item ~ ~ ~ { Item: { id: "minecraft:diamond", count: 1 } };

  // For loops.
  for (int i = 0; i < 100; i++) {
    /give @a diamond 1;
  }

}

// Define a function that returns an int/takes in parameters.
// Function parameters can have default values.
int add(int a, int b = 5) {
  return a + b;
}

// Create a global variable;
int cool_number = 2;

void baz() {
  // Use a function that returns an int/takes in parameters.
  cool_number = add(cool_number, 5) + 1;

  int x;

  // You can create a reference with '&' before the name on initialization.
  int &x_reference1 = x;

  // Get the address of a variable with '&' (e.g. '@s my_objective' for int).
  int x_reference2 is "${&x}" = 0;
  // Get just parts of the address with '&[]' and an index (indexed 0..1).
  /tellraw @a { "score": { "name": "${&[0]x}", "objective": "${&[1]x}" } };

  // A variable can be declared as 'static'. This means it will only be
  // initialized the first time this function is called (only once).
  static int funcCallCount = 0;
  funcCallCount++;
}

// Allow an input to be mutated by passing it as a reference. If optimization is
// off this will copy the arguments in and then copy them back out afterwards.
void add_1(int& a) {
  a += 1;
}
```

#### NBT Variables

```mcfunc
void foo() {
  // Create an 'nbt' variable. 'nbt' variables must be assigned with snippets.
  nbt x = `{ a: 1b, b: "hello world" }`;

  // 'object', 'array', and 'string' are all aliases of 'nbt';
  const object my_object = `{ c: "hi!" }`;
  const array my_array = `[1, 2, 3]`;
  const string my_string = `"hello"`;

  // You can treat an 'nbt' variable like an object and get a value from a key.
  my_object->`c` = `"bye!"`;
  // This also works with 'nbt' variables as a key.
  const string key = `c`;
  my_object->key = `"hi again!"`;

  // You can index into an array with a known index or expression.
  my_array[0] = `100`;
  // You can also index into an array by searching (this is a vanilla feature).
  const array arr = `[{value: 5, name: "bob"}, {value: 10, name: "joe"}]`;
  const nbt joeInfo = arr[`{value: 10}`];

  // Get the address of a variable using '&' (e.g. 'entity @s Pos' for nbt).
  int x_reference is "${&x}" = 0;
  // Get just parts of the address with '&[]' and an index (indexed 0..2).
  /tellraw @a { "$[0]x": "$[1]x", "nbt": "$[2]x" };

  // Use 'nbt' methods. You can pass variables to any of these functions.
  x = `[4, 1, 2]`;
  const int len = x.length();
  x.append(x[0]);
  x.pop(0);
  x.insert(2, `3`);
  x.prepend(`0`);
  x = `{}`;
  x.merge(`{ a: 100, b: "hi" }`);
  if (x.has(`a`))
    x.remove(`a`);

  // References are actually useful with 'nbt' variables.
  object numbers = `[10, 20, 30, 40, 50]`;
  nbt& theNumber20 = numbers[1];
}
```

#### Macros

```mcfunc
void foo() {
  // Macros use 'nbt' variables. If you pass an 'nbt' variable or literal into
  // a command or snippet like it's a snippet variable the value will be
  // inserted into the command with a macro. You should name defined snippets
  // using ALL_CAPS to ensure it's clear what commands use macros and what ones
  // don't.
  const string helloMsg = `"Hello!"`;
  /tellraw @a ${helloMsg};
  /tellraw @a ${`"Goodbye!"`};
  const string name = `"Joey"`;
  if (`entity @s[name=${name}]`)
    /say The name is ${name}!;
}
```

#### Undesigned Language Additions

Here's a list of possible future changes that have not been fully designed or
thought through yet.

- Structs/classes with methods, constructors, and destructors.
- The ability to dynamically pass around and call functions.
- Switch case statements.

### Future Compiler Additions

#### Serialization

Instead of fully compiling, object files can be generated. Object files are
binary files meant to be read incredibly quickly to speed up compilation. Object
files stores a version (so that it can be rejected if it was compiled with an
outdated version of the compiler) and the file's symbol table in a binary
format. Imported symbols are not stored since they should get resolved during
the linking stage.

This can be enabled with the `-c` flag (for "compile") since compilation is
happening but not linking. This outputs all object files into the directory
specified with the `-o` flag.
