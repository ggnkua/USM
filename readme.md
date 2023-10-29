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
