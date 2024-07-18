# MCFunc

MCFunc is a language and compiler for Minecraft Java Edition data packs. MCFunc
offers major improvements on the vanilla `.mcfunction` syntax in an attempt to
make the data pack creation process easier, faster, more modular, and less
bug-prone.

<details open><summary>Table of Contents</summary>

- [Installation](#installation)
- [Language](#language)
  - [Syntax Basics](#syntax-basics)
  - [Expose a Namespace](#expose-a-namespace)
  - [Functions](#functions)
  - [Commands and Scopes](#commands-and-scopes)
  - [Writing Files](#writing-files)
  - [Imports](#imports)
- [CLI App](#cli-app)
  - [Direcly Passing Source Files](#direcly-passing-source-files)
  - [Changing the Output Directory](#changing-the-output-directory)
  - [Adding an Input Directory](#adding-an-input-directory)
  - [Linking a Library Directory](#linking-a-library-directory)
  - [Build System](#build-system)
  - [Hot Reloading](#hot-reloading)
- [Recommended Workflow](#recommended-workflow)
- [Building This Project From Source](#building-this-project-from-source)

</details>

<details open><summary>Example Code</summary>

Here is an example data pack for a simple werewolf game that can be written
entirely in 1 file:

```mcfunc
expose "werewolf_game";

load void init() {
  /scoreboard objectives add werewolf_game.is_dead deathCount;
  /scoreboard objectives add werewolf_game.game_info dummy;
  /scoreboard players set #game_is_running werewolf_game.game_info 0;

  /gamerule doImmediateRespawn true;
}

tick void gameLoop() {
  // if the game is running check for dead players
  /execute if score #game_is_running werewolf_game.game_info matches 1 run: {
    /execute as @a[scores={werewolf_game.is_dead=1..}] run: {
      /scoreboard players reset @s werewolf_game.is_dead;
      /gamemode spectator @s;
      /title @s { "text": "You Died", "color": "red" };

      /execute if entity @s[tag=isWerewolf] run: {
        /tag @s remove isWerewolf;
        villagersWin();
      }

      /execute if entity @s[tag=isVillager] run: {
        /tag @s remove isVillager;
      }

      // if all villagers are dead
      /execute unless entity @a[tag=isVillager] run:
        werewolfWins();
    }
  }

  // if the game has not stopped then update the clock
  /execute if score #game_is_running werewolf_game.game_info matches 1 run: {
    /execute if score #time_remaining werewolf_game.game_info macthes ..0 run:
      villagersWin();

    /scoreboard players remove #time_remaining werewolf_game.game_info 1;
  }
}

void villagersWin() {
  /title @a "Villagers Win";
  stopGame();
}

void werewolfWins() {
  /title @a "Werewold Wins";
  stopGame();
}

void stopGame() {
  /scoreboard players set #game_is_running werewolf_game.game_info 0;
}

// exposed function can be run in game as "werewolf_game:start_game"
void startGame() expose "start_game" {
  /tellraw @a "The game is starting...";

  /scoreboard players set #game_is_running werewolf_game.game_info 1;
  /scoreboard players set #time_remaining werewolf_game.game_info 2400;

  // reset player states from last game
  /gamemode survival @a;
  /scoreboard players reset @a werewolf_game.is_dead;
  /clear @a;
  /effect clear @a;
  /effect give @a minecraft:instant_health 1 255 true;
  /effect give @a minecraft:saturation 1 255 true;

  // all players get a wooden sword
  /give @a minecraft:wooden_sword;

  /tag @a remove isWerewolf;
  /tag @a remove isVillager;

  // a random player becomes the werewolf
  /execute as @r run:
    initializeWerewolf();

  initializeAllVillagers();

  /spreadplayers ~ ~ 10 10 true @a;
}

void initializeWerewolf() {
  /tag @s add isWerewolf;

  /title @s title ["You are a ", { "text": "Werewolf", "color": "red" }];
  /tellraw @s "Kill the villagers.";

  // werewolves also get a better sword
  /give @s minecraft:netherite_sword;
}

void initializeAllVillagers() {
  /execute as @a[tag=!isWerewolf] run: {
    /tag @s add isVillager;

    /title @s title ["You are a ", { "text": "Villager", "color": "green" }];
    /tellraw @s "Survive the werewolf for 2 minutes.";
  }
}
```

</details>

---

## Installation

*todo...*

---

## Language

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

Whitespace quantity does not matter in commands. Consucutive spaces, tabs, and
newlines are all merged into a single space. The following two statements are
effectively the same but the 1st is far more readable.

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

Every data pack needs a namespace for its resources to live under (like the
`foo` in `foo:bar`).

```mcfunc
expose "my_namespace";
```

> [!TIP]
> This statement must appear exactly 1 time during compilation. It's recommended
> that you expose your namespace in your "main" file.
> 
> A namespace can contain lowercase letters `a`-`z`, digits `0`-`9`, underscores
> `_`, dots `.`, and dashes `-` (although it's recommended that you avoid dots
> `.` and dashes `-`). Namespaces cannot start with `zzz__.`.

### Functions

Declare a function with the function's return type (always `void` for now), the
function's name, and a pair of parenthesis `()`.

```mcfunc
// Declare the function 'myFunction'.
void myFunction();
```

We can define the function by putting the code we want the to run in curly
braces `{}` after the parenthesis `()`. We'll have this function run a `/say`
command. Commands in MCFunc start with a slash `/` and end with a semicolon `;`.

```mcfunc
void myFunction() {
  /say Hello world!;
}
```

> [!TIP]
> You do not need a semicolon `;` at the end of a scope `{}` (like a function
> definition) because it's implied.

Here's all of the code for a "*Hello world!*" data pack:

```mcfunc
expose "my_namespace";

void myFunction() {
  /say Hello world!;
}
```

The function we defined above will not appear in-game as
`my_namespace:myFunction`. Instead, it will appear under the namespace
`zzz__.my_namespace` with an arbitrary name (like `zzz__.my_namespace:f_0`).
This is done because it hides implementation details and makes it less likely
that the end-user runs functions they shouldn't.

To give our function a name in-game we have to use the `expose` keyword after
the parenthesis `()` with a function path. We'll expose our function as
`hello_world`. The function we'll see in game will now be called
`my_namespace:hello_world`.

```mcfunc
expose "my_namespace";

void myFunction() expose "hello_world" {
  /say Hello world!;
}
```

> [!NOTE]
> The function path should be a string of elements where each element can
> contain letters `a`-`z`, digits `0`-`9`, underscores `_`, dots `.`, and dashes
> `-` (although it's recommended that you avoid dots `.` and dashes `-`) with
> slashes `/` separating each element (e.g. `"foo/bar/baz"`). An element cannot
> be named with only dots `.` (e.g. `"..."` is invalid). Every slash `/` must
> have an element around it.
> 
> The `expose` keyword does not need to appear for an exposed function's
> definition if it has already appeared on a declaration (and vice versa). This
> is valid:
> 
> ```mcfunc
> void myFunction() expose "hello_world";
> 
> void myFunction() {
>   /say Hello world!;
> }
> ```

To call a function you just put the name of the function with parenthesis `()`
as a statement.

```mcfunc
void myFunction() expose "hello_world" {
  sayHelloWorld5Times();
}

void sayHelloWorld5Times() {
  /say Hello world!;
  /say Hello world!;
  /say Hello world!;
  /say Hello world!;
  /say Hello world!;
}
```

> [!TIP]
> Functions do not need to be declared before they are called.

The `tick` keyword means that function should be placed in the `#minecraft:tick`
function tag. The same is true for the `load` keyword and the `#minecraft:load`
function tag.

```mcfunc
tick void tickFunction() {
  /say this runs every tick;
}

load void loadFunction() {
  /say this runs every time the data pack loads/reloads;
}
```

> [!NOTE]
> `tick` functions run before `load` functions. This may cause problems if a
> `tick` function relies on things a `load` function sets up.
> 
> The `tick` and `load` keywords can both be applied to the same function. They
> must appear before the return type (e.g. `void tick` is invalid). They also
> must appear on every declaration/definition of a function.

### Commands and Scopes

If you want to run a function within some context (like after an `execute`
commmand), you can. Putting a colon `:` after a `run` argument (`run:`) of a
command will break out of that command and allow you to chain another statement
(like a function call).

```mcfunc
void giveAllPlayersStuff() expose "give_all_players_stuff" {
  /execute as @a run: giveStuff();
}

void giveStuff() {
  /give @s diamond_sword 1;
  /give @s shield 1;
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
can write those files into your data pack with the `file` keyword and a file
path.

```mcfunc
file "loot_table/my_loot_table.json";
```

Directly writing a file's contents can be done with an equal sign `=` and the
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
to the file path of an input file.

```mcfunc
file "loot_table/my_loot_table_1.json" = "my_loot_table_1.json";
```

> [!NOTE]
> The file path for a file that's being written starts in the namespace folder
> (e.g. `data/foo/` if the namespace was set to `foo`).
>
> Files being copied in must be input files or must exist inside of an input
> directory or library.

### Imports

You can import members of other MCFunc files into this data pack with the
`import` keyword.

```mcfunc
import "foo.mcfunc";
```

Importing a file allows you to use all members of that file that are not marked
as `private`.

```mcfunc
// foo.mcfunc

// This function can be used by files that import "foo.mcfunc".
void foo() {
  bar();
}

// This function *cannot* be used by files that import "foo.mcfunc".
private void bar() {
  /say bar;
}
```

> [!NOTE]
> File members declared with the `private` keyword must be defined in the same
> file.
> 
> The `private` keyword must appear before the return type (e.g. `void private`
> is invalid). It also must appear on every declaration/definition of a member.
> 
> Files imported by imported files are not accessible in an importing file (e.g.
> if `a` imports `b` and `b` imports `c`, members of `c` cannot be used in `a`
> unless `a` also imports `c`).

---

## CLI App

To get help info you can run `mcfunc` with the `-h` flag.

```sh
# print help info
mcfunc -h
```

### Direcly Passing Source Files

Run `mcfunc` followed by a list of source files to generate a data pack in the
`./data` folder of the current directory (it will be made if it doesn't exist).

```sh
# builds './src/main.mcfunc' into './data'
mcfunc ./src/main.mcfunc
```

Files passed in this way can be imported by the file name without the path.

```mcfunc
import "main.mcfunc";
```

Files that do not have the `.mcfunc` extension will not be compiled but they can
be copied into the data pack with the `file` keyword.

```sh
# builds './src/main.mcfunc' into './data'
mcfunc ./src/foo.mcfunc ./src/bar.json
```

```mcfunc
expose "example";

// copies './src/bar.json' into the data pack as './data/example/baz.json'
file "baz.json" = "bar.json";
```

### Changing the Output Directory

You can change the output directory with the `-o` flag. If this flag appears
multiple times its last appearance will be used. If left unspecified `./data`
will be used.

```sh
# builds './src/main.mcfunc' into './build'
mcfunc ./src/main.mcfunc -o ./build
```

### Adding an Input Directory

You can add an input directory with the `-i` flag. This is similar to directly
passing every file in the directory to the compiler. *This is evaluated*
*recursively!* This means that if you set an input directory `./foo`, MCFunc
files in `./foo/bar` will also be compiled.

```sh
# builds files in the current directory into './data'
mcfunc -i .
```

The difference between this and manually passing every file in a directory to
the compiler is that sub-directories are preserved for the import path (e.g. if
`./src` was set as an import path `./src/foo/bar.mcfunc` would need to be
imported with `"foo/bar.mcfunc"` not just `"bar.mcfunc"`).

The output directory will be ignored if it's inside of/is an input directory.

### Linking a Library Directory

You can link a library directory with the `-l` flag. This works the same as the
`-i` flag except that files from this directory will only be able to import
files that are also inside of the directory. Additionally, source files from
outside directories will be able to import from this one. The `-l` flag is
basically `-i` except the directory is isolated until the linking stage of
compilation.

```sh
# builds files in './src' into './data' and links the './my_lib' library
mcfunc -i ./src -l ./my_lib
```

Files or directories explicitly included (either listed or included with `-i`)
from a library will not be isolated (e.g. if the directory `./my_lib` was linked
in as a library but the file `./my_lib/foo.mcfunc` was separately listed,
`./my_lib/foo.mcfunc` will not be isolated and will be free to be imported by
other code). This is useful if you want to use an API/library because you can
block the importing of API/library files and only allow a single interface file
to be accessed (like a C header file). This also reduces the possiblility of
name collisions since only public members of files explicitly brought into the
global scope can cause any interference.

As an example, let's say your main code was in `./src` but that code uses
functions from a library in the `./math` directory. The `./math` directory may
have a bunch of implementation files but it should also have a single file that
declares everything meant to be imported (e.g. `./math/math.mcfunc`). This file
could be passed to the compiler separately and the rest of the library files
could be linked in.

```sh
# builds files in './src' and './math' into './data' separately allows
# './math/math.mcfunc' to be imported by files in './src' as '"math.mcfunc"'
mcfunc -i ./src -l ./math ./math/math.mcfunc
```

### Build System

If you run `mcfunc` with none of the above arguments it will search for a
`build.jsonc` file in the current directory. This file should contain an array
of arguments that the compiler should use. This way you don't need to type out a
long command every single time you want to compile.

```sh
# builds using arguments from a 'build.jsonc' file
mcfunc
```

Here is an example of a `build.jsonc` file that will build a data pack from the
files in `./src/` into `./data/`:

```jsonc
// build.jsonc
[ "-i", "./src" ]
```

> [!NOTE]
> The main difference between `json` and `jsonc` files is that `jsonc` files
> allow C-style comments.

### Hot Reloading

The `--hot` flag will make the compiler enter an interactive mode that tries to
re-compile the data pack every 2.5 seconds if any source files have changed.
This mode can be exited by pressing `Q`.

```sh
# builds in "hot reload" mode using arguments from a 'build.jsonc' file
mcfunc --hot
```

## Recommended Workflow

The recommended directory structure for MCFunc projects is this:

```
.
├── data/
├── libs/
├── src/
├── build.jsonc
└── pack.mcmeta
```

- `data` is your output directory (where the compiled data pack will go).
- `libs` should contain sub-directories for libraries or APIs. You likely don't
  need this for simple projects.
- `src` is where all of your `.mcfunc` files and any resource files (e.g. loot
  tables) should go.
- `build.jsonc` is here so you don't have to type out a long build command every
  time you want to compile.
- `pack.mcmeta` holds info about your data pack for the game
  ([here's a generator](https://misode.github.io/pack-mcmeta/)).

Ideally you're directly working inside of a data pack folder in the `datapacks`
directory of a Minecraft save. That way you can take advantage of things like
hot reloading for testing.

To send your data pack folder to a friend just zip the contents of the project
(only `data` and `pack.mcmeta` if they don't need the source code) and send it
to them.

As for your `build.jsonc` file, you can use this if you don't need any libraries
(this also means you don't need the `libs` folder):

```jsonc
// build.jsonc
[ "-i", "./src" ]
```

If you have libraries you'll need to add a bit to this file for each of them.
As an example, let's say we have a library in the `./libs/math` directory with
an interface file `./libs/math/math.mcfunc`:

```jsonc
// build.jsonc
[
  "-i", "./src",
  "-l", "./libs/math", "./libs/math/math.mcfunc"  // import as "math.mcfunc"
]
```

For more complicated projects with multiple namespaces you may want to write
your own build script since you'll need to compile each namespace with a
separate build command (I'd recommend Python for that).

## Building This Project From Source

To build this project you must be using a Unix system like Linux or Mac OS (use
[WSL](https://learn.microsoft.com/en-us/windows/wsl/install) if you're on
Windows).

You need to have Python 3, CMake, Git, and either GCC or Clang installed. I'd
recommend you have Make installed since it has been tested here but other CMake
generators may work. Make sure your compiler is updated to work with C++ 17.

You can build the project by running the [build.py](./build.py) script from the
root project directory. This will also run all of the tests.

```sh
# build the project and run all tests
./build.py
```

> [!IMPORTANT]
> Make sure you aren't already in the build directory when you run the build
> script.

The build script will build the executables `mcfunc` and `run_tests` in the
`./build` directory. You can run these executables from the root project
directory.

```sh
# run the main executable (debug)
./build/mcfunc
```

To build in release mode run the python script with `release` as an argument.
Build files will end up in `./release` instead of `./build`.

```sh
# build the project in release mode and run all tests
./build.py release
```

```sh
# run the main executable (release)
./release/mcfunc
```
