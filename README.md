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

## Copyright

License: MIT

## Requirements

- Compiler which supports C++17
- CMake ≥ 3.10

