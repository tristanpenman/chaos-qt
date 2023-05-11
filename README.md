# Chaos Qt

A level editor for Sonic The Hedgehog Mega Drive / Genesis ROMs.

This is a C++ port of Brett Kosinski's [Chaos level editor](https://github.com/fancypantalons/chaos). It can be used to modify Sonic The Hedgehog 2 ROM files. It can also be used to view levels from Sonic The Hedgehog 3 ROMs.

Here's a screenshot of the app being used to edit Metropolis Zone, Act 1 from Sonic The Hedgehog 2:

![Editing Metropolis Zone, from Sonic The Hedgehog 2](./doc/metropolis.png)

Levels can also be zoomed in and out while editing, as seen in this screenshot of Hill Top Zone:

![Editing Hill Top Zone, from Sonic The Hedgehog 2](./doc/hilltop.png)

## Progress

The version of the code you're looking at now is a partially complete C++ / Qt port.

## Build

This version of the code can be built using Qt versions 5 and 6. Assuming you have Qt and CMake installed, the basic build steps are as follows:

    git clone --recurse-submodules https://github.com/tristanpenman/chaos.git
    cd chaos/qt
    mkdir build
    cd build
    cmake ..
    make

This will compile both the main application and a test suite.

On macOS, this will build an application bundle called `Chaos.app`. The test suite is a single binary called `ChaosTest`.

## Documentation

Included in the [doc](./doc) directory is Brett Kosinski's [write up](./doc/kosinski.txt) of the compression algorithm used.

## License

This code is licensed under the MIT License.

See the LICENSE file for more information.
