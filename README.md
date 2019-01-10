# WaitFreeCollections

<table>
    <tr>
        <td><strong>master branch</strong></td>
        <td>
            <a href="https://ci.appveyor.com/project/CBenoit/waitfreecollections/branch/master"><img src="https://ci.appveyor.com/api/projects/status/sucn29if5de65t01/branch/master?svg=true"></a>
        </td>
    </tr>
    <tr>
        <td><strong>develop branch</strong></td>
        <td>
            <a href="https://ci.appveyor.com/project/CBenoit/waitfreecollections/branch/develop"><img src="https://ci.appveyor.com/api/projects/status/sucn29if5de65t01/branch/develop?svg=true"></a>
        </td>
    </tr>
    <tr>
        <td><strong>License</strong></td>
        <td>
            <a href="https://opensource.org/licenses/MIT"><img src="https://img.shields.io/badge/License-MIT-yellow.svg"></a>
        </td>
    </tr>
</table>

A header-only library providing wait-free collections such as hash map and double ended queue (deque).

## Wait-Free Hash Map

An implementation of a wait-free hash map as described in the article

> P. Laborde, S. Feldman et D. Dechev, « A Wait-Free Hash Map », <br>
> International Journal of Parallel Programming, t. 45, n o 3, p. 421-448, 2017, issn : 1573-7640. <br>
> doi : https://doi.org/10.1007/s10766-015-0376-3

See [unordered_map example](./examples/unordered_map_example.cpp) in `examples` folder.

## Double ended queue (Deque)

WIP

## Requirements

- Compiler which supports C++17
- CMake ≥ 3.10

## Minimal exemple

```cpp
#include <iostream>
#include <thread>

#include <wfc/unordered_map.hpp>

constexpr int nbr_threads = 16;

int main()
{
    wfc::unordered_map<std::size_t, std::size_t> m(4, nbr_threads, nbr_threads);

    std::array<std::thread, nbr_threads> threads;
    for (std::size_t i = 0; i < nbr_threads; ++i)
    {
        threads[i] = std::thread([&m, i]() {
            m.insert(i, i);
        });
    }

    for (auto& t: threads)
    {
        t.join();
    }

    m.visit([](std::pair<const std::size_t, std::size_t> p) {
        std::cout << '[' << p.first << '-' << p.second << "]\n";
    });

    return 0;
}
```

This should output:

```
[0-0]
[1-1]
[2-2]
[3-3]
[4-4]
[5-5]
[6-6]
[7-7]
[8-8]
[9-9]
[10-10]
[11-11]
[12-12]
[13-13]
[14-14]
[15-15]
```

## How to import the library using CMake

To include the library, you may copy / paste everything in a subfolder (such as `externals`) or use a git submodule.

Then, in you CMakeLists.txt file, you just have to do something similar to:

```cmake
cmake_minimum_required(VERSION 3.10)
project(MyProject)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(externals/WaitFreeCollections)

find_package(Threads REQUIRED) # you'll probably need that one too

add_executable(MyTarget src/main.cpp)
target_link_libraries(MyTarget Threads::Threads WaitFreeCollections)
```

At this point, the project hierarchy looks like:

```
|- CMakeLists.txt
|- src/
    |- main.cpp
|- externals/
    |- WaitFreeCollections/
        |- ...
```

You may try to use the minimal example above as your `main.cpp`.

Also, since this is a header-only library, you may copy / paste the content of the include folder in your
project to achieve similar results (maybe a little bit less clean though).

## Build targets

You don't need to actually build the library beforehand to use it in your project due to the "header-only" nature of it.
The build targets in this repository are for tests, exemples and code formatting.
These are good to know should you contribute to this project or play with the exemples.

First, you need to create a sub-directory to run `cmake` from it. Then you can build using `make`.

```
$ mkdir build
$ cmake .. -DWFC_BUILD_ALL=1
```

By default, everything is build in Release mode. You can pass `-DCMAKE_BUILD_TYPE=Debug` to `cmake` to ask Debug builds.
This will also activate useful features for developing purposes such as the holy warnings.
The `-DWFC_BUILD_ALL=1` parameter will tell CMake to include all our targets in the build process.

CMake will generate several targets that you can build separately using the `--target` parameter:

- `UnorderedMapExample1`
- `UtilityTests`
- `UnorderedMapTests`
- `Clang-format`

For instance, to build the `UnorderedMapTests` run 

```
$ cmake --build . --target UnorderedMapTests
```

You can also build everything by not giving any specific target.
Produced executables are inside the `bin` folder.

Note that the `Clang-format` target does not produce anything.
It just run the clang formatter.
Also the target needs to be called explicitly.

For more details, see the [CMakeLists.txt](CMakeLists.txt) file.

## Copyright

License: MIT

Main contributors:

- [Jérôme Boulmier](https://github.com/Lomadriel)
- [Benoît Cortier](https://github.com/CBenoit)
