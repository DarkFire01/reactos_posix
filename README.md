# reactos_posix

this is the POSIX C runtime,
the reskit/BSD userland, the X11R6.9 / Motif 2.2.2 / CDE client stack, and the
runtime data slipstreamed onto the ReactOS CD.

The core subsystem itself lives in the ReactOS source tree and stays there:


## Building

Clone this next to the ReactOS source
tree so the two are siblings :3 :

```
somewhere/
├── reactos/         (the ReactOS tree with subsystems/posix)
└── reactos_posix/   (this repo)
```

`reactos/subsystems/posix/CMakeLists.txt` detects `../reactos_posix` at
configure time and `add_subdirectory()`s it, so everything here is configured
and built as part of the normal ReactOS build and lands on the `bootcd` ISO.

Userland targets are named `posix_<name>` (emitting `<name>.exe`) to avoid
colliding with ReactOS Win32 targets; `sh` keeps its plain name.
