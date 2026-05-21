USM
===

A small command-line tool that packages Atari ST `.PRG` / `.TOS` programs
into 128 KB cartridge ROM images — usable in emulators (Hatari, STEem) or
burned to real cart hardware.

USM produces one of three cart layouts per program in the same cart:

| Mode                              | Flag        | What ends up in ROM                                     | When to use                                              |
| --------------------------------- | ----------- | ------------------------------------------------------- | -------------------------------------------------------- |
| Default                           | *(none)*    | A 236-byte stub loader + the verbatim PRG file          | The normal case — works for any well-formed PRG          |
| Default + LZSS compressed         | `-z`        | A 304-byte LZSS stub + the LZSS-compressed PRG          | Larger or repetitive PRGs you want to squeeze in         |
| Classic                           | `-c`        | TEXT + DATA only, relocated to ROM addresses            | Tiny position-independent programs / early-boot code     |

Multi-program carts (multiple PRGs chained via `CA_NEXT`) work in any mode
and can freely mix compressed and uncompressed entries.

---

Quick start
-----------

Build USM:

```sh
./build_mac_linux.sh        # macOS / Linux
# or
build_mingw.bat             # Windows (MinGW)
```

Make a cart from a single PRG:

```sh
./usm GAME.ROM GAME.PRG
```

That's it — `GAME.ROM` is a 128 KB cart image you can hand to Hatari:

```sh
hatari --cartridge GAME.ROM
```

Inside Hatari you'll see a `c:` drive on the desktop. Open it and
double-click `GAME.PRG` to run.

For **STEem Engine**, add `-s` so the output has the 4-byte STEem
prefix STEem expects, and give the file a `.STC` extension by
convention:

```sh
./usm -s GAME.STC GAME.PRG
```

`GAME.STC` is the same 128 KB cart plus a 4-byte zero prefix — STEem
loads it via its cartridge insertion dialog.

### Running carts on real hardware

A USM-built `.ROM` image is a standard 128 KB Atari ST cart image.
You can burn it to an EEPROM and use it in a physical cart board, but
the safest and most flexible option for real hardware is the
[**SidecarTridge Multidevice**](https://sidecartridge.com/products/sidecartridge-multidevice-atari-st/)
in **ROM Emulation** mode: drop the `.ROM` file onto its storage and
the device presents it to the ST as if it were a real cart, with no
soldering, no UV erase cycles, and no risk of bricking a chip. The
`.STC` variant is for the STEem emulator only — physical-hardware
deployments use the plain `.ROM` output.

---

Usage
-----

```
usm [global flags] image_filename <program [per-program flags]>...
```

### Global flags (before the image filename)

| Flag       | Meaning                                                                                          |
| ---------- | ------------------------------------------------------------------------------------------------ |
| `-s`       | Prepend the 4-byte STEem Engine prefix (STEem-compatible cart format)                            |
| `-d`       | Diagnostic cartridge — only one program allowed, executed by the OS at `$FA0004` after reset    |
| `-c`       | Classic mode — write TEXT+DATA directly to ROM, relocated at cart-build time                     |
| `-z`       | Compress each PRG with LZSS (auto-falls-back to uncompressed when it doesn't help)               |
| `-bXXXXXX` | Set the BSS address for classic mode (hex, default `$20000`)                                     |
| `-fY`      | Set the OS init flag, where `Y` is one of:                                                       |
|            | &nbsp;&nbsp;`0` — execute prior to display memory + interrupt vector init                        |
|            | &nbsp;&nbsp;`1` — execute just before GEMDOS is initialized                                      |
|            | &nbsp;&nbsp;`3` — execute prior to boot disk                                                     |
|            | &nbsp;&nbsp;`5` — application is a Desk Accessory                                                |
|            | &nbsp;&nbsp;`6` — application is not a GEM application                                           |
|            | &nbsp;&nbsp;`7` — application needs parameters                                                   |

### Per-program flags (after each PRG path)

`-b`, `-f`, and `-z` can be repeated **after** each program filename to
override the global default for that one program only. `-c` switches the
cart-build mode for the remaining programs once it appears.

### Defaults

- `-b` default: `$20000` (used by classic mode)
- `-f` default: `0` — but with no `-f` flag at all, `CA_INIT` is written
  as `0` (entirely), which tells TOS *not* to auto-run the program. The
  program shows up as a file on the cart's `c:` drive and the user
  launches it from the desktop.

### Conflict rules

- `-z` with `-c` is rejected — classic mode runs the program directly
  from ROM, so there's no destination to decompress to.
- `-z` with `-d` is rejected — diagnostic carts execute with no TOS
  context, but the LZSS decompressor needs `Malloc`.

---

Examples
--------

### Single-program cart (default mode)

```sh
./usm GAME.ROM GAME.PRG
```

Produces a 128 KB cart that boots to the desktop with `GAME.PRG` visible
on the cart's `c:` drive. Double-click to run.

### Multi-program cart

```sh
./usm GAMES.ROM PACMAN.PRG TETRIS.PRG SNAKE.PRG
```

All three programs appear on `c:`. The user picks one to run; the others
sit dormant in ROM until launched.

### Compressed single-program cart

```sh
./usm -z GAME.ROM GAME.PRG
```

USM tries LZSS compression. If the compressed entry is smaller than the
uncompressed one, it ships compressed; if not, it auto-falls-back to
uncompressed. Either way you get a working cart and a one-line note:

```
==> GAME.ROM
      + GAME.PRG: compressed 57436 -> 41696 (72.6%)
```

or

```
==> GAME.ROM
      + GAME.PRG: no savings (110% entry), shipping uncompressed
```

### Compressed multi-program cart

```sh
./usm -z BUNDLE.ROM GAME1.PRG GAME2.PRG GAME3.PRG
```

Compression is attempted for each program independently. A cart might
mix compressed and uncompressed entries based on which ones LZSS could
actually shrink — that's normal and fully supported by the chain.

### Mixed compression (per-program override)

```sh
./usm BUNDLE.ROM GAME1.PRG -z GAME2.PRG GAME3.PRG
```

Only `GAME2.PRG` is compressed (the `-z` after its filename applies just
to that one); the other two ship uncompressed.

Or the reverse — global `-z`, with no easy way to opt out per-file (by
design — the auto-fallback handles the cases where you wouldn't actually
want compression):

```sh
./usm -z BUNDLE.ROM GAME1.PRG GAME2.PRG GAME3.PRG
```

### Classic mode (program runs directly from ROM)

```sh
./usm -c TINY.ROM TINY.PRG
```

USM relocates TEXT+DATA at cart-build time so the program runs in place
in ROM. BSS lands at `$20000` in RAM by default; override with `-b`:

```sh
./usm -c -b40000 TINY.ROM TINY.PRG
```

Classic mode only works reliably for position-independent or
absolute-address programs that don't rely on PC-relative BSS access.
Most realistically: this is what you'd use for tiny boot loaders /
chooser routines, not for full applications.

### Diagnostic cart (auto-executes at reset)

```sh
./usm -d -c DIAG.ROM DIAG.PRG
```

The OS jumps to `$FA0004` after reset — your program is executed
immediately, before GEMDOS is up. `-d` requires `-c` because the
default-mode stub assumes a TOS Pexec environment that doesn't exist at
diagnostic-cart time. Only one program is allowed.

### Auto-launch at boot

```sh
./usm -f3 GAME.ROM GAME.PRG
```

`-f3` sets `CA_INIT`'s "execute prior to boot-disk init" bit. TOS calls
the cart at boot before showing the desktop, useful for custom launchers
or boot-time utilities. Without an `-f` flag, the program waits to be
launched manually from the cart's `c:` drive (the typical case).

### STEem-Engine-compatible cart

```sh
./usm -s GAME.STC GAME.PRG
```

`-s` prepends 4 zero bytes to the output so STEem accepts it.

---

How it works — the three cart modes
-----------------------------------

### Default mode (the common case)

For each PRG, USM emits a 34-byte `CA_HEADER` followed by a small 68k
**stub loader** (236 bytes), followed by the verbatim PRG file. At launch
the stub:

1. `Mshrink`s the basepage TOS hands it down to 256 bytes;
2. `Malloc`s the largest free chunk for a fresh TPA;
3. Builds a complete Pexec-style basepage (`p_lowtpa`, `p_hitpa`,
   `p_tbase`, etc.) from scratch;
4. Copies the PRG from ROM into RAM at `$100(a5)`;
5. Zero-fills BSS (honouring the PRGFLAGS fastload bit);
6. Applies PRG relocations;
7. `JMP`s to the program's entry point.

No privileged instructions, so the stub runs in either supervisor mode
(auto-init at `-f3`) or user mode (manual launch from `c:`).

### Default + compressed (`-z`)

Same shape as the default mode but the stub is replaced by a 304-byte
LZSS-aware variant. Cart layout per compressed entry:

```
[ CA_HEADER (34 B) ]
[ LZSS stub (304 B) ]
[ uncompressed_size LONG (4 B, big-endian) ]
[ compressed payload ]
```

The decompressor runs after Mshrink+Malloc, writing the original PRG
bytes straight into the TPA at `$100(a5)`. The rest of the launch flow
(BSS zero / relocation / `JMP`) is identical to the uncompressed path.

### Classic mode (`-c`)

No stub. TEXT+DATA are written directly to ROM with all in-program
absolute addresses relocated to `$FA0000`-range cart addresses at
build time. BSS references point at a hardcoded RAM address (`-b`,
default `$20000`). The program runs in place from ROM.

---

Compression — LZSS-12-4
-----------------------

USM ships a custom LZSS variant. Both the host-side encoder (in `usm.c`)
and the 68k decompressor (in `prg_loader_compressed.s`) are written from
scratch — no external library.

**Bitstream**: a sequence of ≤ 9-byte blocks. Each block is one flag
byte followed by up to 8 tokens. The flag's bits, MSB-first, classify
each token:

- bit = 0 → next 1 byte is a literal.
- bit = 1 → next 2 bytes are a back-reference, big-endian, packed as
  `(offset - 1) << 4 | (length - 3)`. Offset range: 1..4096 (12 bits).
  Length range: 3..18 (4 bits).

**Auto-fallback** is load-bearing, not paranoia. Roughly half of
real-world PRGs (especially packed demos and games) grow under LZSS
because their data is already incompressible. Empirical ratios on a few
test fixtures:

| File         | Original | Compressed | Result                                 |
| ------------ | -------- | ---------- | -------------------------------------- |
| MONST2.PRG   |   24756  |    20762   | shrinks to 83.9% — used                |
| SWITV310.TOS |   57436  |    41696   | shrinks to 72.6% — used                |
| RAID.PRG     |   23318  |    25641   | grows to 110% — auto-falls-back        |
| SYSINFO.PRG  |   58234  |    65458   | grows to 112% — auto-falls-back        |
| hello.prg    |      88  |       77   | data shrinks but the larger LZSS stub  |
|              |          |            | makes the entry grow — auto-falls-back |

The "entry" comparison includes the difference between the 236-byte
uncompressed stub and the 304-byte compressed stub.

---

Caveats
-------

- Only use single-file PRG programs. Programs that try to load external
  files at runtime will fail (the cart has no filesystem).
- Sanity checks have improved (input-file validation, magic-number
  rejection, bounded relocation walker) but USM is still permissive
  about strange PRG headers.
- Classic mode (`-c`): programs that access BSS via PC-relative
  addressing won't work — there's no signal in the PRG relocation table
  for those references, so we can't rewrite them. If your program needs
  this, use the default mode instead.
- Maximum input file size is 128 KB (the cart itself is 128 KB and we
  need room for at least the `CA_HEADER` and a stub).

---

Building
--------

### The host-side tool (`usm`)

```sh
./build_mac_linux.sh        # macOS / Linux — gcc -O2 usm.c -o usm
build_mingw.bat             # Windows — MinGW gcc
# Or open usm.sln in Visual Studio.
```

The host build only needs a C compiler — `usm.c` is a single translation
unit with no library dependency.

### The 68k stub loaders (`prg_loader.s`, `prg_loader_compressed.s`)

You only need this step if you actually edit one of the assembly stub
sources. The byte arrays `prg_loader[]` and `prg_loader_compressed[]`
inside `usm.c` are hand-copied snapshots of `prg_loader.bin` and
`prg_loader_compressed.bin` — they don't drift on their own.

The cleanest way to assemble the stubs is through the
[**atarist-toolkit-docker**](https://github.com/sidecartridge/atarist-toolkit-docker),
which ships a ready-made `vasm` (plus the rest of the GNU m68k Atari ST
cross-toolchain) inside a Docker container, callable via a thin host-side
wrapper named `stcmd`. Follow the toolkit's README to install it
(typically: clone the repo, run its installer, pull the image; the
installer puts `stcmd` on your `PATH`). With it installed:

```sh
# Make sure stcmd is on PATH and Docker is running, then from the
# usm repo root:
STCMD_NO_TTY=1 STCMD_QUIET=1 stcmd vasm \
    -quiet -Fbin -o prg_loader.bin prg_loader.s

STCMD_NO_TTY=1 STCMD_QUIET=1 stcmd vasm \
    -quiet -Fbin -o prg_loader_compressed.bin prg_loader_compressed.s
```

`STCMD_NO_TTY=1` and `STCMD_QUIET=1` keep the wrapper headless and
silent — useful from scripts or CI. The output `.bin` files are flat
68k binaries (no PRG header, no relocs) suitable for embedding as raw
byte arrays.

After assembly, regenerate the C array literal. A small Python one-liner
that matches `usm.c`'s existing formatting:

```sh
python3 - <<'PY' > /tmp/array.c
data = open('prg_loader.bin','rb').read()       # or prg_loader_compressed.bin
print("unsigned char prg_loader[] =")           # or prg_loader_compressed[]
print("{")
for i in range(0, len(data), 16):
    cells = ['0x{:02X}'.format(b) for b in data[i:i+16]]
    line = ' ,'.join(cells)
    print(line + ('' if i + 16 >= len(data) else ','))
print("};")
PY
```

…then paste the contents of `/tmp/array.c` over the corresponding array
literal in `usm.c`, rebuild `usm`, and run `tests/run-tests.sh` to make
sure nothing regressed. The CI in `.github/workflows/build.yml` runs the
same round-trip + cart-build sanity tests on Linux and macOS.

If you'd rather not use Docker at all, any standalone `vasm` binary
(Motorola syntax — `vasmm68k_mot`) works the same way; just drop the
`stcmd` wrapper. The Atari toolkit Docker image is the path of least
resistance because it pins a known-good vasm version reproducibly across
all hosts.

---

Testing
-------

Helper scripts under `tests/`:

| Script                            | What it does                                                       |
| --------------------------------- | ------------------------------------------------------------------ |
| `tests/build-roms.sh`             | For each PRG in `tests/binaries/`, build a single-program cart     |
| `tests/build-multi-rom.sh`        | Build a 3-program uncompressed cart (`build/THREE.ROM`)            |
| `tests/build-multi-rom-z.sh`      | Build a 3-program **compressed** cart (`build/THREE_Z.ROM`)        |
| `tests/run-roundtrip.sh`          | Round-trip every fixture through the LZSS encoder + decoder        |
| `tests/run-tests.sh`              | Build uncompressed + compressed carts for every fixture, check 128 KB |
| `tests/hatari.sh <name>`          | Launch a built cart in Hatari (`build/<name>.ROM`)                 |

There's also a hidden debug command for the LZSS encoder/decoder pair:

```sh
./usm -T some-file.prg
# some-file.prg: 24756 -> 20762 bytes (83.9%) OK
```

This compresses the file in memory, decompresses it, and byte-compares —
useful when debugging encoder/decoder changes.

The `tests/binaries/` directory is `.gitignore`d (third-party PRGs vary
in redistribution rights, so you supply your own). `tests/fixtures/`
holds a tiny tracked Hello-World PRG that exercises the auto-fallback.

---

Thank yous
----------

Diego Parrilla for supplying the Github actions to automatically build binaries for all platforms, and some other fixes in the code.
tIn/Newline for the original stub loader source code.
