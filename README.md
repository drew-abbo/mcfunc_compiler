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
  - [Hot Reloading](#hot-reloading)
  - [Additional Flags](#additional-flags)
- [Recommended Workflow](#recommended-workflow)
  - [Project Structure](#project-structure)
  - [Build System (Make)](#build-system-make)
- [Building This Project From Source](#building-this-project-from-source)
  - [Requirements](#requirements)
  - [Python Build Script](#python-build-script)
  - [Building With CMake](#building-with-cmake)
  - [Running the Executables](#running-the-executables)
  - [Using an IDE](#using-an-ide)
    - [Visual Studio / CLion](#visual-studio--clion)
    - [Xcode](#xcode)
    - [VSCode](#vscode)

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
`zzz__.my_namespace` with an arbitrary name. This is done because it hides your
implementation details from the user, making it less likely that someone runs
functions they shouldn't.

To give our function a name in-game we have to use the `expose` keyword after
the parenthesis `()` with a function path. We'll expose our function as
`hello_world`. The function we'll see in-game will now be called
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
> be named `.` or `..`. Every slash `/` must have an element on either side of
> it.
>
> The `expose` keyword can only appear on a function's definition.

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

The `tick` keyword means that a function should be placed in the
`#minecraft:tick` function tag. The same is true for the `load` keyword and the
`#minecraft:load` function tag.

```mcfunc
tick void tickFunction() {
  /say this runs every tick;
}

load void loadFunction() {
  /say this runs every time the data pack loads/reloads;
}
```

> [!NOTE]
> The `tick` and `load` keywords can both be applied to the same function. They
> must appear before the return type (i.e. `void tick` is invalid). They also
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

> [!NOTE]
> There must be at least one whitespace character before and after `run:` and
> it must appear outside of any parenthesis or strings.

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
file "loot_table/my_loot_table.json" = `{
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
> (e.g. `file "loot_tables/my_loot_table_1.json"` would actually write to
> `foo/loot_tables/my_loot_table_1.json` for a namespace `foo`).
>
> Files being copied in must be input files or must exist inside of an input
> directory or library.

### Imports

You can import functions from other MCFunc files and use them in the current one
with the `import` keyword.

```mcfunc
import "foo.mcfunc";
```

Importing a file allows you to use any functions that file has marked as
`public`.

```mcfunc
// foo.mcfunc

// This function can be used by files that import "foo.mcfunc".
public void foo() {
  bar();
}

// This function *cannot* be used by files that import "foo.mcfunc".
void bar() {
  /say bar;
}
```

Functions marked as `public` are global. This means that if file `a` and file
`b` both define a public function `foo` (`public` keyword used) there will be a
conflict (even if there's no importing between the two).

On the other hand, if multiple files both define a non-public function `foo`
(`public` keyword not used) then there will not be a conflict (even if there is
importing between them) so long as a *public* function `foo` is never declared.

The advantage of this is that you can split a function's declaration from its
definition and have a clean interface file for a library or API (like a C header
file).

```mcfunc
// doSomethingComplicated.mcfunc

// This file can be imported and people can look at it to see how the contained
// functions should be used.

import "src/doSomethingComplicated.mcfunc";

// Does something complicated.
// Call when you want something complicated to be done.
public void doSomethingComplicated();
```

```mcfunc
// src/doSomethingComplicated.mcfunc

// This file can implement the functions in the other file without cluttering it
// with private helper functions or implementation details.

public void doSomethingComplicated() {
  // big complicated implementation goes here
}
```

> [!NOTE]
> The `public` keyword must appear before the return type (i.e. `void public`
> is invalid). It also must appear on every declaration/definition of a
> function.

---

## CLI App

### Direcly Passing Source Files

Run `mcfunc` followed by a list of source files to generate a data pack in the
`./data` folder of the current directory (it will be made if it doesn't exist).

```sh
# builds './src/main.mcfunc' into './data'
mcfunc ./src/main.mcfunc
```

Files passed in this way can be imported by their file name (without the parent
path).

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

Input files cannot be inside of the output directory.

### Changing the Output Directory

You can change the output directory with the `-o` flag. This flag cannot appear
multiple times. If left unspecified `./data` will be used.

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
# builds files in the `./src` directory into './data'
mcfunc -i ./src
```

The difference between this and manually passing every file in a directory to
the compiler is that sub-directories are preserved for the import path (e.g. if
there was a file `./src/foo/bar.mcfunc` and you used the flag `-i ./src`, the
file would need to be imported as `"foo/bar.mcfunc"`, not just `"bar.mcfunc"`).

The output directory cannot be inside of an input directory.

### Hot Reloading

The `--hot` flag will make the compiler enter an interactive mode that tries to
re-compile the data pack every 2.5 seconds if any source files have changed.
This mode can be exited by pressing `Q`.

```sh
# builds './src' into './data' in "hot reload" mode
mcfunc -i ./src --hot
```

### Additional Flags

| Flag             | Purpose                                        |
| ---------------- | ---------------------------------------------- |
| `-v` `--version` | Print version info.                            |
| `-h` `--help`    | Print help info.                               |
| `--no-color`     | Disable styled output (no color or bold text). |

## Recommended Workflow

### Project Structure

The recommended directory structure for MCFunc projects is this:

```
.
├── data/
├── libs/
├── src/
├── Makefile
└── pack.mcmeta
```

- `data` is your output directory (where the compiled data pack will go).
- `libs` should contain sub-directories for libraries or APIs. You likely don't
  need this for simple projects.
- `src` is where all of your `.mcfunc` files and any resource files (e.g. loot
  tables) should go.
- `Makefile` lets you just run `make` to compile ([more info](#build-system-make)).
- `pack.mcmeta` holds info about your data pack for the game
  ([here's a generator](https://misode.github.io/pack-mcmeta/)).

Ideally you're directly working inside of a data pack folder in the `datapacks`
directory of a Minecraft save. That way you can take advantage of things like
hot reloading for testing.

### Build System (Make)

If you don't want to manually write out a long build command every time you want
to compile your data pack you'll need a build system. I'd recommend
[Make](https://www.gnu.org/software/make/)
([Windows download here](https://gnuwin32.sourceforge.net/packages/make.htm)).

To use Make (once you have it installed), create a file called `Makefile` and
give it some basic targets:

```Makefile
# set the build command to build './src' into './data'
BUILD_CMD = mcfunc -i ./src

# runs the build command when you run `make`
all:
	${BUILD_CMD}

# runs the build command in "hot reload" mode when you run `make hot`
hot:
	${BUILD_CMD} --hot
```

> [!WARNING]
> Make sure the `Makefile` is using tabs (not spaces) for indentation.

Once you have this set up you should just be able to just run `make` and your
data pack should build.

## Building This Project From Source

### Requirements

Ensure you have [CMake](https://cmake.org) (make sure it's on the path, i.e.
`cmake --version` works in your terminal), a CMake
[generator](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html),
and a C/C++ compiler supported by CMake and your generator.

Once you have the repository cloned you can build with the
[Python build script](#python-build-script), or you can
[build manually with CMake](#building-with-cmake). Check out
[how to run the compiled executables](#running-the-executables) once you've
built successfully.

You can also [use an IDE](#using-an-ide) (like Visual Studio) if you'd prefer.

### Python Build Script

If you have Python installed you can build using the [build.py](./build.py)
script. If you're on a POSIX OS (e.g. Linux or MacOS) you should be able to just
run `./build.py`, otherwise run the script with either `python3` or `python`.

```sh
python3 build.py
```

If you don't get an error message from the script then you're done.

Using the Python build script you can easily switch between debug and release
builds by adding the `--debug` and `--release` flags (debug by default). The
script will automatically reconfigure CMake if it needs to.

```sh
python3 build.py --release
```

You can add `-p` or `--parallel` to compile with all CPU cores. Keep in mind
that this can make error messages and warnings less cohesive.

```sh
python3 build.py -p
```

For more help info run the script with `-h` or `--help`.

> [!NOTE]
> If you are on Windows the script will use
> [`Visual Studio 17 2022`](https://visualstudio.microsoft.com/) as it's
> generator. Otherwise it will use
> [`Unix Makefiles`](https://www.gnu.org/software/make/). If you want to use
> the Python script make sure you have the correct generator installed.

### Building With CMake

Start by configuring CMake in a build folder called `build`.

```sh
cmake -B build
```

> [!NOTE]
> If you're using a single configuration generator like `Unix Makefiles` (you
> probably are if you're on MacOS/Linux) then you can configure for release mode
> by adding `--DCMAKE_BUILD_TYPE=Release`.
>
> ```sh
> cmake -B build --DCMAKE_BUILD_TYPE=Release
> ```

Now we can build the project by running this:

```sh
cmake --build build
```

> [!NOTE]
> If you're using a multi-configuration generator like `Visual Studio 17 2022`
> (you probably are if you're on Windows) then you can build in release mode by
> adding `--config Release`.
>
> ```sh
> cmake --build build --config Release
> ```

### Running the Executables

Successfully built executables will be in your build directory.

> [!NOTE]
> If you're on Windows using Visual Studio as your generator then your
> executables will be in the `Debug` sub-folder of the `build` directory (or
> `Release` if you built in release mode).

You can run your compiled executable like this:

On MacOS/Linux (using `Unix Makefiles` or a similar generator):

```sh
./build/mcfunc
```

On Windows (using `Visual Studio 17 2022` or a similar generator):

```ps1
.\build\Debug\mcfunc.exe    # Using Visual Studio on Windows (debug)
.\build\Release\mcfunc.exe  # Using Visual Studio on Windows (release)
```

The same goes for the `run_tests` executable (just replace `mcfunc` with
`run_tests`).

### Using an IDE

#### Visual Studio / CLion

Just open the project folder with the editor. CMake should automatically be
configured.

#### Xcode

To use Xcode, configure CMake and then open the `.xcodeproj` folder.

```sh
cmake -B build -G "Xcode"
open build/mcfunc.xcodeproj
```

#### VSCode

You can either use the integrated terminal and the build info above or you can
use the [CMake Tools](vscode:extension/ms-vscode.cmake-tools) extension to build
and run with `shift+F5` (you can just build with `F7`).
