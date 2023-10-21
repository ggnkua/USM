USM
---

Small tool to ease the creation of Atari ST cart images.

Usage
=====

From the command line:

`usm [s] image_filename <list_of_programs_to_add>`

where *s* is optional and allows creation of STEem Engine compatible carts, *image_filename* is the filename of the cart image and *list_of_programs_to_add* is a list of ST PRG programs to add to the image.

Caveats (boy, here we go)
=========================

- Only use single file PRG programs. Programs that try to load external files will not work.
- TODO: Some sanity checks are performed, but the program is far from bullet proof
- TODO: Programs that contain BSS should normally not work. For now, we point the bss to the hardcoded address of $20000. This will be user definable in the future. (Unfotunately it doesn't seem possible to do this automatically. If anyone has an idea on how to do this, happy to discuss!)
- On BSS: programs that access the BSS using PC relative code will most likely fail
- TODO: CA_INIT bitfields are not yet available to the user, so we simply add the programs as if they're meant to be run from desktop as GEM applications
- TODO: Date and Time are still set to bogus vaules


