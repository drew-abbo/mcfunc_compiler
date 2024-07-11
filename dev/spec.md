# MCFunc

- [Language](#language)
  - [Future Additions](#future-additions)
    - [Imports](#imports)
    - [Snippets](#snippets)
    - [Scheduling Code](#scheduling-code)
    - [Control Flow](#control-flow)
    - [Integer Data Type](#integer-data-type)
    - [NBT Variables](#nbt-variables)
    - [Selectors](#selectors)
    - [Undesigned Language Additions](#undesigned-language-additions)
- [CLI App](#cli-app)
- [Advanced User Documentation](#advanced-user-documentation)
  - [Namespaces](#namespaces)
  - [Function File Naming Patterns](#function-file-naming-patterns)
  - [Writing Files](#writing-files)
- [Stages of Compilation](#stages-of-compilation)

## Language

Here are all of the features of the MCFunc language:

### Syntax Basics

MCFunc uses C-style comments:

```mcfunc
// 1-line comments look like this.

/*
Block comments look like this (multi-line).
*/
```

All statements in MCFunc must end with a semicolon `;`.

```mcfunc
expose "foo";
```

Whitespace quantity does not matter. Consucutive spaces, tabs, and newlines are
all merged into a single space. The following two statements are effectively the
same but the 1st is far more readable.

```mcfunc
// Statement 1:
/summon creeper ~ ~ ~ {
  NoAI: 1b,
  ExplosionRadius: 10b,
  Fuse: 0,
  ignited: 1b
};

// Statement 2:
/summon creeper ~ ~ ~ { NoAI: 1b, ExplosionRadius: 10b, Fuse: 0, ignited: 1b };
```

> [!NOTE]
> Whitespace *is* preserved inside of strings (`"hello there"` is not the same
> as `"hello    there"`).

### Expose a Namespace

The first thing in every file needs to be the namespace that the code in this
file will live under once it's generated (like the `foo` in `foo:bar`). The
namespace can contain lowercase letters `a`-`z`, digits `0`-`9`, underscores
`_`, dots `.`, and dashes `-` (although it's recommended that you avoid dots `.`
and dashes `-`). You can only expose 1 namespace per file.

```mcfunc
// 1st thing needs to be a namespace.
expose "my_namespace";
```

### Functions

Declare a function with the function's return type (always `void` for now), the
function's name, and a pair of parenthesis `()`.

```mcfunc
// Declare the function 'myFunction'.
void myFunction();
```

We can define the function by putting the code we want the to run in curly
braces `{}`. We'll put a `/say` command with a "*Hello world!*" message.
Commands in MCFunc start with a slash `/` and end with a semicolon `;`.

```mcfunc
void myFunction() {
  /say Hello world!;
}
```

> [!TIP]
> You do not need a semicolon `;` at the end of a scope `{}` (like after a
> function definition).

Here's all of the code for a "*Hello world!*" data pack:

```mcfunc
expose "my_namespace";

void myFunction() {
  /say Hello world!;
}
```

The function we defined above will not appear in-game as
`my_namespace:myFunction`. Instead, it will appear under the namespace
`zzz.my_namespace` with an arbitrary name (like `zzz.my_namespace:f_0`). This
is done because it hides implementation details and makes it less likely that
the end-user runs functions they shouldn't.

To give our function with a name in-game we have to use the `expose` keyword
after the parenthesis `()` with a function path. The function path is a string
of elements where each element can contain letters `a`-`z`, digits `0`-`9`,
underscores `_`, dots `.`, and dashes `-` (although it's recommended that you
avoid dots `.` and dashes `-`) with slashes `/` separating each element (e.g.
`"foo/bar/baz"`). We'll expose our function as `"hello_world"`. The function
we'll see in game will now be called `my_namespace:hello_world`.

```
expose "my_namespace";

void myFunction() expose "hello_world" {
  /say Hello world!;
}
```

> [!NOTE]
> The `expose` keyword does not need to appear for an exposed function's
> definition if it has already appeared on a declaration:
>
> ```mcfunc
> void myFunction() expose "hello_world";
> 
> void myFunction() {
>   /say Hello world!;
> }
> ```

To call a function you just put the name of the function and parenthesis `()` as
a statement. Functions must be declared and/or defined *before* they are called.

```mcfunc
void sayHelloWorld5Times() {
  /say Hello world!;
  /say Hello world!;
  /say Hello world!;
  /say Hello world!;
  /say Hello world!;
}

void myFunction() expose "hello_world" {
  sayHelloWorld5Times();
}

```

You can mark a function with the `tick` and/or `load` keywords if you want it
to run every tick or when the data pack's reloads.

```mcfunc
tick void tickFunction() {
  /say this runs every tick;
}

load void loadFunction() {
  /say this runs every time the data pack loads/reloads;
}
```

> [!CAUTION]
> `tick` functions run before `load` functions. This may cause problems if a
> `tick` function relies on things a `load` function sets up.

### Commands and Scopes

If you want to run a function within some context (like after an `execute`
commmand), you can. Putting a colon `:` after a `run` argument of a command
(e.g. `run:`) will break out of that command and allow you to run a statement.

```mcfunc
void giveStuff() {
  /give @s diamond_sword 1;
  /give @s shield 1;
}

void giveAllPlayersStuff() expose "give_all_players_stuff" {
  /execute as @a run: giveStuff();
}
```

Instead of calling a function, you can also open a scope after `run:`. This
allows you to run multiple commands after a `run` argument without defining a
separate function.

```mcfunc
void giveAllPlayersStuff() expose "give_all_players_stuff" {
  /execute as @a run: {
    /give @s diamond_sword 1;
    /give @s shield 1;
  }
}
```

> [!TIP]
> You do not need a semicolon `;` at the end of a scope `{}`.

This can be used to make more complicated operations way more readable.

```mcfunc
void saveAllPlayerPositionsInArray() {
  /data modify storage my_namespace:storage player_positions set value [];

  /execute as @a run: {
    /data modify storage my_namespace:storage player_positions append value {};
    /data modify storage my_namespace:storage player_positions[-1].UUID set from
      entity @s UUID;
    /data modify storage my_namespace:storage player_positions[-1].pos set from
      entity @s pos;
  }

  /tellraw @a "Player position data saved.";
}
```

### Writing Files

Sometimes your data pack needs other kinds of resources (e.g. loot tables). You
can write those files into your data pack directly with the `file` keyword and a
file path (which starts inside the namespace directory).

You can directly write the file's contents here with an equal sign `=` and the
file's contents as a snippet in backticks <code>&#96;</code>.

```mcfunc
file "loot_table/my_loot_table.json" =
`{
  "pools": [{
    "rolls": 1,
    "entries": [{ "type": "minecraft:item", "name": "minecraft:stone" }]
  }]
}`;
```

Alternatively, you could copy the file by assigning it (with an equal sign `=`)
to the file path of a source file which exists inside an input directory.

```mcfunc
file "loot_table/my_loot_table_1.json" = "my_loot_table_1.json";
```

### Future Additions

#### Imports

```mcfunc
// The 'private' keyword here tells the compiler that files outside of the same
// directory as this one cannot import this file. For example, if this file was
// 'src/my_lib/foo.mcfunc', 'src/my_lib/bar.mcfunc' could import this file but
// 'src/baz.mcfunc' could not. This allows us to easily hide source files behind
// an API without using header files.
private export "foo";

// Imports need to appear before any definitions but after 'export'. This allows
// use to use any public members of another file.
import "bar.mcfunc";

// The 'private' keyword here means that this function can only be used in this
// file, even if another file imports this one.
private void foo() {
  bar();
}

// You *can* import members from another namespace.
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
int add(const int a, const int b) {
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
}

// Allow an input to be mutated by passing it by as a reference.
void add_1(int& a) {
  a += 1;
}
```

#### NBT Variables

```mcfunc
void foo() {
  // Create an nbt variable. You can only assign nbt variables with snippets
  // and there are no operators.
  nbt x = `{ a: 1b, b: "hello world" }`;

  // 'object', 'array', and 'string' are all aliases of 'nbt';
  const object my_object = `{ c: "hi!" }`;
  const array my_array = `[1, 2, 3]`;
  const string my_string = `"hello"`;

  // Get the value based on an object's key with a snippet.
  my_object->`c` = `"bye!"`;
  // This also works with nbt variables as a key.
  const string key = `c`;
  my_object->key = `"hi again!"`;

  // Index into an array with a known index or expression.
  my_array[0] = `100`;
  // Index into an array by searching (this is a vanilla feature).
  const array arr = `[{value: 5, name: "bob"}, {value: 10, name: "joe"}]`;
  value = arr[`{value: 10}`];

  // Get the address of a variable using '&' (e.g. 'entity @s Pos' for nbt).
  int x_reference is "${&x}" = 0;
  // Get just parts of the address with '&[]' and an index (indexed 0..2).
  /tellraw @a { "$[0]x": "$[1]x", "nbt": "$[2]x" };

  // Use nbt methods. You can pass variables to any of these functions.
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

  // Use macros.
  const string hi_msg = `"Hello!"`;
  with (hi_msg, bye_msg = `"Goodbye!"`) {
    /tellraw @a ${hello_msg};
    /tellraw @a ${bye_msg};
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

#### Undesigned Language Additions

Here's a list of possible future changes that have not been fully designed or
thought through yet.

- Modules (so you could have `foo::bar()`) with public and private members.
- Structs/classes with methods, constructors, and destructors.
- The ability to dynamically pass around and call functions.
- Switch case statements.

## CLI App

Run `mcfunc` followed by a list of source files to generate a data pack in the
`data/` folder of the current directory (will make it if it doesn't exist).

```sh
# builds './src/main.mcfunc' into './data/'
mcfunc ./src/main.mcfunc
```

You can change the output directory with the `-o` flag. If this flag appears
multiple times its the last appearance will be used. If left unspecified
`./data/` will be used.

```sh
# builds './src/main.mcfunc' into './build/'
mcfunc ./src/main.mcfunc -o ./build
```

You can add an input directory with the `-i` flag. This tells the compiler that
all files with the `.mcfunc` extension inside of the set directory should be
compiled. *This is evaluated recursively!* This means that if you set an input
directory `./foo/`, MCFunc files in `./foo/bar/` will also be compiled. If the
output directory is inside of an input directory it will be ignored.

```sh
# builds all '.mcfunc' files in the current directory into './data/'
mcfunc -i .
```

The `--hot` flag will make the compiler enter an interactive mode that tries to
re-compile the data pack every 2.5 seconds if any source files have changed.
This mode can be exited by pressing `Q`.

```sh
mcfunc --hot
```

If you run `mcfunc` with no arguments it will search for a `build.json` file.
This file should contain an array of arguments.

```sh
# looks for arguments from a 'build.json' file.
mcfunc
```

Here is an example of a `build.json` file that will build a data pack from the
files in `./src/` into `./data/`:

```json
// build.json
[ "-i", "./src" ]
```

To get all of this help info use the `-h` flag.

```sh
# print help info
mcfunc -h
```

## Advanced User Documentation

### Namespaces

You can set the a namespace with the `expose` keyword and a string that contains
only lowercase letters `a-z`, digits `0-9`, underscores `_`, dots `.`, and
dashes `-` (although it's recommended you avoid dots and dashes). The namespace
has to be the 1st thing in every file (excluding comments and whitespace) and
must appear exactly 1 time. Multiple namespaces cannot exist within the same
file.

```mcfunc
expose "foo";
```

This sets the string of characters that appear before `:` for `.mcfunction`
files in-game (e.g. `foo` in `foo:bar`).

### Function File Naming Patterns

Any `.mcfunction` files created that don't represent exposed functions will
follow the following naming pattern where `$HIDDEN_NAMESPACE` is the exposed
namespace with `zzz__` in front of it and `$SYMBOL_ID` is an arbitrary
hexadecimal number used for identification.

```
$HIDDEN_NAMESPACE:f_$SYMBOL_ID
```

Exposed functions instead follow the following naming pattern where `$NAMESPACE`
is the exposed namespace and `$EXPOSED_NAME` is the text in quotes after the
`expose` keyword in the function definition.

```
$NAMESPACE:$EXPOSED_NAME
```

So the following block of code could output the functions `foo:my_func` and
`zzz__foo:f_1`:

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

### Writing Files

When writing a file the output file path is a relative path and is a
subdirectory of `$OUTDIR/$NAMESPACE` where `$OUTDIR` is the directory set with
the `-o` compilation flag and `$NAMESPACE` is the exposed namespace.

```mcfunc
// writes 'loot_table/my_loot_table_1.json' with contents
file "loot_table/my_loot_table_1.json" =
`{
  "pools": [{
    "rolls": 1,
    "entries": [{ "type": "minecraft:item", "name": "minecraft:stone" }]
  }]
}`;
```

You can also copy the file's contents from another file. Do this with by putting
a string with the source file's path after `=` instead of a snippet. The source
file will be searched for in all input directories that were set with the `-i`
compilation flag. The first one it finds will be used. The file path must be
within an input directory (can't do `../`). If no input directories are set or
the file cannot be found then compilation will fail. If the default settings
were used on compilation `loot_table/my_loot_table_1.json` would be copied from
`./src/my_loot_table_1_source.json`.

```mcfunc
file "loot_table/my_loot_table_1.json" = "my_loot_table_1_source.json";
```

You can also just put the output file and no `=` symbol to tell the compiler
that the definition for that file has to exist somehwere. This can be useful if
you use a resource that should be defined in another file.

```mcfunc
// ensures that 'loot_table/my_loot_table_1.json' is defined somewhere
file "loot_table/my_loot_table_1.json";
```

## Stages of Compilation

1. **Tokenization** - All input files are read through and convered into tokens
  (small syntax elements like a semicolon `;`).
1. **Namespace and Import Resolution** - Tokens are peeked into to evaluate the
  initial `export` statement and imports (which have to appear at the top of the
  file). All files are labeled with the namespace their code lives under and
  import dependency trees are generated. These trees lay out what needs to be
  evaluated first and also allows for later files that do not depend on each
  other to be evaluated in parallel (e.g. if `foo` imports `bar`, `bar` must be
  evaluated before `foo`).
1. **Syntax Analysis** - Each file is individually evaluated in the order
  specified by the import dependency trees. The syntax is checked for validity,
  a symbol table is created, and code is grouped into more broad actions like
  "write file" or "run command". This stage is done in parallel for all files.
1. **Linking** - All files are able to be evaluated together, allowing for
  things like imports to work. If any symbols are missing after this step
  compilation fails.
1. **Optimization (Optional)** - With a symbol table that's complete with
  definitions lots of optimizations (like inlining) can occur. This stage can be
  enabled or disabled. Other minor optimizations may occur in previous steps
  regardless.
1. **Translation** - Translation is done to convert every operation into a file
  write operation. This includes things like giving functions addresses and
  converting all non-command operations (like function calls) into commands.
  The output of this step should be a list of file paths with contents that need
  to be generated.
1. **Code Generation** - The data pack is generated from the symbol table.
