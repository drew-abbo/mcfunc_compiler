# MCFunc

## Language

Here are all of the features of the MCFunc language:

```mcfunc
// 1st thing needs to be a namespace.
expose "my_namespace";

// Define functions ('void' is used to facilitate variables in the future).
void foo() {
    // run commands like normal, just with a '/' at the start and a ';' at the
    // end.
    /execute as @a run say hi;

    // Commands can span multiple lines because whitespace is reduced.
    /summon creeper ~ ~ ~ {
        NoAI: 1b,
        powered: 1b,
        ExplosionRadius: 10b,
        Fuse: 0,
        ignited: 1b
    };  // ';' indicates the end of the statement.
}

void bar() {
    // Run code after commands with 'run:'.
    /execute as @a run: foo();

    // Implicitly define mcfunction files by opening a scope.
    /execute as @s run: {
        /say hi again again;
    }
}

// Define a function that's exposed to the user.
void main() expose "main" {
    /say this is an exposed function!;
    bar();
}

// Manually write a file.
file "loot_table/my_loot_table_1.json" =
`{
    "pools": [{
        "rolls": 1,
        "entries": [{ "type": "minecraft:item", "name": "minecraft:stone" }]
    }]
}`;

// Copy a file from an input source directory.
file "loot_table/my_loot_table_1.json" = "my_loot_table_1.json";

// The 'tick' and 'load' keywords can be used on function definitions to add
// them to the '#minecraft:tick' and/or '#minecraft:load' function tags. This
// works around the fact that '.mcfunc' files can only directly write into their
// namespace folders (reduces conflicts from multiple namespaces being compiled
// into the same data pack).
tick void on_tick() {
    /effect give @a speed 1 0 true;
}
load void on_tick() {
    /tellraw @a "This data pack is installed!";
}
```

### Future Additions

#### Snippets

```mcfunc
void foo() {
    // Define a snippet (like a compile time macro) (definition is scoped).
    snippet MSG = `hello`;

    // Snippet primitives just say "make sure this snippet exists already".
    snippet MSG;

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

#### Imports

```mcfunc
// allow this file to use definitions from another file.
import "bar.mcfunc";

void foo() {
    bar();
}
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
    /execute at @s run: while !`block ~ ~ ~ #minecraft:air`
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
# builds './src/main.mcfunc' into 'data/'
mcfunc ./src/main.mcfunc
```

You can also just run `mcfunc` with no arguments to tell the compiler to include
all files with the `.mcfunc` file extension in the current directory's `src/`
folder (or any sub-directory of '`./src/`').

```sh
# builds all '.mcfunc' files in './src/' into './data/'
mcfunc
```

You can change the output directory with the `-o` flag. If this flag appears
multiple times the last directory will be used.

```sh
# builds all '.mcfunc' files in './src/' into './build/'
mcfunc -o ./build
```

You can add an input directory with the `-i` flag. This can be used if you want
to include a library in another folder or if you want to use something other
than `./src/`. If a directory cannot be found it will be ignored. Files inside
of the output directory are completely ignored.

```sh
# builds all '.mcfunc' files in the current directory into './data/'
mcfunc -i .
```

Even when you pass no arguments, default arguments are passed. Here are the
default arguments where `$STDLIB` is replaced with the absolute path to the
standard library directory (operating system dependant).

```sh
# default arguments (implicitly passed)
mcfunc $STDLIB/built_in.mcfunc -i $STDLIB -i ./src -o ./data
```

If you want to disable the default arguments use `--clear-previous-flags` which
tells the compiler to ignore all flags before the it.

```sh
# build without the standard library or built-in file
mcfunc --clear-previous-flags ./src/main.mcfunc -o ./data
```

> [!WARNING]
> This is not recommended. You'll lose any features that come from the standard
> library (including built-in features), you'll lose the default output
> directory, and you'll need to list all input files or manually set an input
> directory.

All of this functionality allows you to use build systems like `Make` for more
complex projects that may have multiple namespaces in the same data pack.

## Standard Library

### `built_in.mcfunc`

This file defines a bunch of basic functionality and features.

```mcfunc
// std/built_in.mcfunc

/* ONCE SNIPPETS ARE IMPLEMENTED:

snippet NAMESPACE;  // ensure hard coded snippet exists
snippet HIDDEN_NAMESPACE;  // ensure hard coded snippet exists

*/
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
files in-game (e.g. `foo` in the in-game function `foo:bar`).

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
