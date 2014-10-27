# Proof of Concept for separate user code

## Setup

First put the core in DFU mode:

```
cd main
make clean all program-dfu
```

Enter DFU mode again:

```
cd ../apps
make clean all program-app-dfu
```

The core is now flashed with system firmware and a user app. In this demo,
the firmware waits for input from Serial before invoking the
`foo()` function in the user app, which prints a hello world message and sets
the LED to pink.


## Steps remaining to turn this into production-ready:

 - add `module_dispatch()` function that is called with an int. It dispatches
to init(), setup() or loop(). 
 - add init() function to copy initialized data from FLASH into RAM, and also to
zero initialize the BSS section.
 - add a callback in the system code to allow the start of heap to be set, and
set it to the end of BSS. 
 - allow gc-sections in the linker, since without it the system firmware is too large.
   but ensure most of our code is kept via KEEP() declarations in the linker script.
 - add a fingerprint to the compiled system code, which is baked into both the system code
    and the user code. At runtime, do not call user code functions if the IDs do not match.
 (This is better than crashing!)







