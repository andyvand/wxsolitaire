# wxSolitaire

A cross-platform implementation of classic Klondike Solitaire built with C++ and [wxWidgets](https://www.wxwidgets.org/).

![GPL-2.0](https://img.shields.io/badge/license-GPL--2.0-blue)

## Screenshots

- Windows:

![](/screenshots/wxsolitaire_win.png)

- macOS:

![](/screenshots/wxsolitaire_macOS.png)

- Linux:

![](/screenshots/wxsolitaire_linux.png)

## Features

- Classic Klondike solitaire gameplay
- Draw one or draw three card modes
- Scoring modes: Standard, Vegas, or None
- Timed game mode
- Undo support
- Drag-and-drop with outline or full bitmap dragging
- 12 card back designs
- Keyboard navigation

## Building

### Prerequisites

- C++11 compiler
- CMake 3.14+
- wxWidgets (core, base, xrc, xml, html components)

### Build steps

```sh
mkdir build && cd build
cmake ..
cmake --build .
```

On macOS this produces a `wxSolitaire.app` bundle. On Windows it builds a GUI executable with the application icon and manifest. On Linux it builds a standard executable.

## Installing

After building, install with:

```sh
cmake --install .
```

On Linux the install includes a `.desktop` file and AppStream metadata for integration with desktop environments and software centres.

## License

This project is licensed under the [GNU General Public License v2.0](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html).

The `bin2c.c` utility is licensed under the BSD 2-Clause license (copyright Rafael Kitover, 2016).
