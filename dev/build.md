# Build Guide (Unix)

## Build With CMake

To set up CMake:

```sh
mkdir build     # make a `build/` directory
cd build        # move into `build/` directory
cmake ..        # set up CMake
```

If any `CMakeLists.txt` files are changed (must be in `build/` directory):

```sh
cmake .         # run CMake (if any CMakeLists files have changed)
```

To build the project (must be in `build/` directory):

```sh
make            # build the project
```

Run tests manually (must be in `build/` directory):

```sh
./run_tests
```

### With VSCode Extensions

Click the play or debug icons in the bottom left corner of the screen if using
the VSCode CMake extension. This extension can also automatically create a
`build/` directory.

## Release Build With CMake

To set up a release build make a different build directory called `release/`.

```sh
mkdir release   # make a `release/` directory
cd release      # move into `release/` directory
```

In the `release/` directory run this setup command instead of just `cmake ..`.
This will build the project with `NDEBUG` and more optimization.

```sh
# set up CMake in release mode
cmake -DCMAKE_BUILD_TYPE=Release ..
```
