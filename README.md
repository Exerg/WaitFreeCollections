# WaitFreeCollections

<table>
    <tr>
        <td><strong>master branch</strong></td>
        <td>
            [![Build status](https://ci.appveyor.com/api/projects/status/sucn29if5de65t01/branch/master?svg=true)](https://ci.appveyor.com/project/CBenoit/waitfreecollections/branch/master)
        </td>
    </tr>
    <tr>
        <td><strong>develop branch</strong></td>
        <td>
            [![Build status](https://ci.appveyor.com/api/projects/status/sucn29if5de65t01/branch/develop?svg=true)](https://ci.appveyor.com/project/CBenoit/waitfreecollections/branch/develop)
        </td>
    </tr>
    <tr>
        <td><strong>License</strong></td>
        <td>
            [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
        </td>
    </tr>
</table>

A header-only library providing wait-free collections such as hash map and linked list.

## Wait-Free Hash Map

An implementation of a wait-free hash map as described in the article

> P. Laborde, S. Feldman et D. Dechev, « A Wait-Free Hash Map », <br>
> International Journal of Parallel Programming, t. 45, n o 3, p. 421-448, 2017, issn : 1573-7640. <br>
> doi : https://doi.org/10.1007/s10766-015-0376-3

See example in `example` folder.

## Copyright

License: MIT

## Requirements

- Compiler which supports C++17
- CMake ≥ 3.10

