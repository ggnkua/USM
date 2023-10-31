USM
===

Small tool to ease the creation of Atari ST cart images.

Usage
-----

From the command line:

`usm [-s] [-d] [-bX] [-fY] image_filename <list of programs to add with optional -b and -f>`

where:

- *-s* is optional and allows creation of STEem Engine compatible carts
- *-d* enables creation of diagnostic cartridges (only one program allowed, which will relocate to $fa0004)
- *-bX* allows setting the BSS address to an address other than the default ($20000)
- *-fY* allows setint the INIT flag. y can be
  - 0       Execute prior to display memory and interrupt vector initialization
  - 1       Execute just before GEMDOS is initialized
  - 3       Execute prior to boot disk
  - 5       Application is a Desk Accessory
  - 6       Application is not a GEM application
  - 7       Application needs parameters
- *image_filename* is the filename of the cart image
- *list of programs to add with optional -b and -f* is a list of ST PRG programs to add to the image, optionally postfixed by a *-b* and/or *-f* to set the parameters for the current file only

More on defaults: The default value for *-b* is $20000, and the default value for *-f* is nothing. Passing *-b* and/or *-f* at the start of the parameter list will override the default values. Passing *-b* and/or *-f* after each program in the list, will override the defaults for that file only.

How it works (briefly)
----------------------

Current (and only) mode of operation is that the program gets the PRG (or PRGs), relocates TEXT and DATA sections to ROM space and BSS to RAM at a hardcoded address, and hopes for the best.

This is not the best (or robust) idea in the world, but at least it made sense when the program was conceptualised. See below for ideas for further development.

Caveats (boy, here we go)
-------------------------

- Only use single file PRG programs. Programs that try to load external files will not work
- TODO: Some sanity checks are performed, but the program is far from bullet proof
- TODO: The BSS start address has to be supplied by user (or accept the default value of $20000). Unfotunately it doesn't seem possible to do this automatically. If anyone has an idea on how to do this, happy to discuss!
- On BSS: programs that access the BSS using PC relative code will most likely fail
- TODO: Date and Time are still set to bogus vaules

Building
--------

Use the provided scripts. Or Visual Studio.

What can be done in the future to improve things
------------------------------------------------

1. Add a small stub in front of each program to malloc enough BSS so if the running program is doing mallocs, it won't overwrite the BSS. Kind of tricky given the many modes of *-fY* and hardcoded BSS address
1. Instead of relocating the program to ROM space, copy it along with relocation information to RAM and *pexec 3* it
1. Keep the whole program in ROM space with relocation information, and at runtime a small stab will relocate it fully to RAM and run it. Again, it could malloc some RAM for this

Thank yous
----------

Diego Parrilla for supplying the Github actions to automatically build binaries for all platforms, and some other fixes in the code.
