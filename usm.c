#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>

#if defined(_WIN32)
#define USM_STAT       _stat
#define usm_stat_t     struct _stat
#else
#define USM_STAT       stat
#define usm_stat_t     struct stat
#endif

unsigned char cart[1024 * 128];
unsigned char prg_temp_buf[1024 * 128];

#if defined (_WIN32)
#pragma pack(2)
#endif
#if defined(__linux__) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__APPLE__) || defined (__COSMOCC__)
#pragma pack(push,2)
#endif

typedef struct
{
    uint32_t    CA_NEXT;            // Pointer to the next application header (or NULL if there are no more).
    uint32_t    CA_INIT;            // Pointer to the application's initialization code. The high eight bits of this pointer have a special meaning as follows:
                                    // Bit Set Meaning
                                    // 0       Execute prior to display memory and interrupt vector initialization.
                                    // 1       Execute just before GEMDOS is initialized.
                                    // 2       (unused)
                                    // 3       Execute prior to boot disk.
                                    // 4       (unused)
                                    // 5       Application is a Desk Accessory.
                                    // 6       Application is not a GEM application.
                                    // 7       Application needs parameters.
    uint32_t    CA_RUN;             // Pointer to application's main entry point.
    uint16_t    CA_TIME;            // Standard GEMDOS time stamp.
    uint16_t    CA_DATE;            // Standard GEMDOS date stamp.
    uint32_t    CA_SIZE;            // Size of application in bytes.
    char        CA_FILENAME[14];    // NULL terminated ASCII filename in standard GEMDOS 8+3 format.
} CA_HEADER;

typedef struct
{
    uint16_t    PRG_magic;  // This WORD contains the magic value (0x601A).
    uint32_t    PRG_tsize;  // This LONG contains the size of the TEXT segment in bytes.
    uint32_t    PRG_dsize;  // This LONG contains the size of the DATA segment in bytes.
    uint32_t    PRG_bsize;  // This LONG contains the size of the BSS segment in bytes.
    uint32_t    PRG_ssize;  // This LONG contains the size of the symbol table in bytes.
    uint32_t    PRG_res1;   // This LONG is unused and is currently reserved.
    uint32_t    PRGFLAGS;   // This LONG contains flags which define certain process characteristics (as defined below).
    uint16_t    ABSFLAG;    // This WORD flag should be non-zero to indicate that the program has no fixups or 0 to indicate it does.Since some versions of TOS handle files with this value being non-zero incorrectly, it is better to represent a program having no fixups with 0 here and placing a 0 longword as the fixup offset.
} PRG_HEADER;
#if defined(__linux__) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__APPLE__) || defined (__COSMOCC__)
#pragma pack(pop)
#endif

#if defined(__linux__) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__APPLE__) || defined (_WIN32) || defined (__COSMOCC__)
#define BYTESWAP_LONG(x) (((x&0xff000000)>>24)|((x&0x00ff0000)>>8)|((x&0x0000ff00)<<8)|((x&0x000000ff)<<24))
#define BYTESWAP_WORD(x) (((x&0xff00)>>8)|((x&0x00ff)<<8))
#else
#define BYTESWAP_LONG(x) x
#define BYTESWAP_WORD(x) x
#endif

unsigned char prg_loader[] =
{
0x28 ,0x6F ,0x00 ,0x04 ,0x2F ,0x3C ,0x00 ,0x00 ,0x01 ,0x00 ,0x2F ,0x0C ,0x42 ,0x67 ,0x3F ,0x3C,
0x00 ,0x4A ,0x4E ,0x41 ,0x4F ,0xEF ,0x00 ,0x0C ,0x70 ,0xFF ,0x2F ,0x00 ,0x3F ,0x3C ,0x00 ,0x48,
0x4E ,0x41 ,0x5C ,0x8F ,0x22 ,0x00 ,0x2F ,0x00 ,0x3F ,0x3C ,0x00 ,0x48 ,0x4E ,0x41 ,0x5C ,0x8F,
0x28 ,0x40 ,0x34 ,0x3C ,0x00 ,0x3F ,0x42 ,0x9C ,0x51 ,0xCA ,0xFF ,0xFC ,0x2A ,0x40 ,0x2A ,0x80,
0x2E ,0x40 ,0xDF ,0xC1 ,0x2B ,0x4F ,0x00 ,0x04 ,0x51 ,0x8F ,0x2F ,0x4D ,0x00 ,0x04 ,0x49 ,0xED,
0x00 ,0x80 ,0x2B ,0x4C ,0x00 ,0x20 ,0x49 ,0xED ,0x00 ,0x30 ,0x2B ,0x4C ,0x00 ,0x2C ,0x49 ,0xFA,
0x00 ,0x8C ,0x41 ,0xEC ,0x00 ,0x1C ,0x43 ,0xED ,0x01 ,0x00 ,0x20 ,0x2C ,0x00 ,0x02 ,0xD0 ,0xAC,
0x00 ,0x06 ,0xD0 ,0xAC ,0x00 ,0x0E ,0xE2 ,0x88 ,0x53 ,0x80 ,0x32 ,0xD8 ,0x51 ,0xC8 ,0xFF ,0xFC,
0x08 ,0x2C ,0x00 ,0x00 ,0x00 ,0x19 ,0x66 ,0x0E ,0x20 ,0x2C ,0x00 ,0x0A ,0xD0 ,0x89 ,0x32 ,0xFC,
0x00 ,0x00 ,0xB0 ,0x89 ,0x6E ,0xF8 ,0x4A ,0x90 ,0x67 ,0x22 ,0x43 ,0xED ,0x01 ,0x00 ,0x22 ,0x09,
0x70 ,0x00 ,0xD3 ,0xD8 ,0xD3 ,0x91 ,0x10 ,0x18 ,0x67 ,0x12 ,0xB0 ,0x3C ,0x00 ,0x01 ,0x66 ,0x06,
0x43 ,0xE9 ,0x00 ,0xFE ,0x60 ,0xF0 ,0xD2 ,0xC0 ,0xD3 ,0x91 ,0x60 ,0xEA ,0x43 ,0xED ,0x01 ,0x00,
0x22 ,0x09 ,0x2B ,0x41 ,0x00 ,0x08 ,0x20 ,0x2C ,0x00 ,0x02 ,0x2B ,0x40 ,0x00 ,0x0C ,0xD2 ,0x80,
0x2B ,0x41 ,0x00 ,0x10 ,0x20 ,0x2C ,0x00 ,0x06 ,0x2B ,0x40 ,0x00 ,0x14 ,0xD2 ,0x80 ,0x2B ,0x41,
0x00 ,0x18 ,0x2B ,0x6C ,0x00 ,0x0A ,0x00 ,0x1C ,0x91 ,0xC8 ,0x4E ,0xD1
};

// LZSS-12-4 compressed-mode stub. Same Mshrink + Malloc + basepage layout
// as prg_loader[], but the ROM->RAM copy phase is replaced by inline LZSS
// decompression. Built from prg_loader_compressed.s. The PC-relative
// `prg_payload` label inside the stub points at a 4-byte big-endian
// uncompressed_size LONG followed by the compressed bytes; usm.c writes
// both into the cart immediately after this stub.
unsigned char prg_loader_compressed[] =
{
0x28 ,0x6F ,0x00 ,0x04 ,0x2F ,0x3C ,0x00 ,0x00 ,0x01 ,0x00 ,0x2F ,0x0C ,0x42 ,0x67 ,0x3F ,0x3C,
0x00 ,0x4A ,0x4E ,0x41 ,0x4F ,0xEF ,0x00 ,0x0C ,0x70 ,0xFF ,0x2F ,0x00 ,0x3F ,0x3C ,0x00 ,0x48,
0x4E ,0x41 ,0x5C ,0x8F ,0x22 ,0x00 ,0x2F ,0x00 ,0x3F ,0x3C ,0x00 ,0x48 ,0x4E ,0x41 ,0x5C ,0x8F,
0x28 ,0x40 ,0x34 ,0x3C ,0x00 ,0x3F ,0x42 ,0x9C ,0x51 ,0xCA ,0xFF ,0xFC ,0x2A ,0x40 ,0x2A ,0x80,
0x2E ,0x40 ,0xDF ,0xC1 ,0x2B ,0x4F ,0x00 ,0x04 ,0x51 ,0x8F ,0x2F ,0x4D ,0x00 ,0x04 ,0x49 ,0xED,
0x00 ,0x80 ,0x2B ,0x4C ,0x00 ,0x20 ,0x49 ,0xED ,0x00 ,0x30 ,0x2B ,0x4C ,0x00 ,0x2C ,0x49 ,0xFA,
0x00 ,0xD0 ,0x20 ,0x1C ,0x20 ,0x4C ,0x43 ,0xED ,0x01 ,0x00 ,0x47 ,0xF1 ,0x08 ,0x00 ,0xB3 ,0xCB,
0x6C ,0x3A ,0x14 ,0x18 ,0x76 ,0x07 ,0xB3 ,0xCB ,0x6C ,0x32 ,0xD4 ,0x02 ,0x65 ,0x08 ,0x12 ,0xD8,
0x51 ,0xCB ,0xFF ,0xF4 ,0x60 ,0xEC ,0x78 ,0x00 ,0x18 ,0x18 ,0xE1 ,0x4C ,0x18 ,0x18 ,0x3A ,0x04,
0xE8 ,0x4C ,0x52 ,0x44 ,0x02 ,0x45 ,0x00 ,0x0F ,0x56 ,0x45 ,0x24 ,0x49 ,0x94 ,0xC4 ,0x53 ,0x45,
0x12 ,0xDA ,0x51 ,0xCD ,0xFF ,0xFC ,0x51 ,0xCB ,0xFF ,0xCE ,0x60 ,0xC6 ,0x49 ,0xED ,0x01 ,0x00,
0x20 ,0x4C ,0x41 ,0xE8 ,0x00 ,0x1C ,0xD1 ,0xEC ,0x00 ,0x02 ,0xD1 ,0xEC ,0x00 ,0x06 ,0xD1 ,0xEC,
0x00 ,0x0E ,0x22 ,0x48 ,0x08 ,0x2C ,0x00 ,0x00 ,0x00 ,0x19 ,0x66 ,0x0E ,0x20 ,0x2C ,0x00 ,0x0A,
0xD0 ,0x89 ,0x32 ,0xFC ,0x00 ,0x00 ,0xB0 ,0x89 ,0x6E ,0xF8 ,0x4A ,0x90 ,0x67 ,0x22 ,0x43 ,0xED,
0x01 ,0x1C ,0x22 ,0x09 ,0x70 ,0x00 ,0xD3 ,0xD8 ,0xD3 ,0x91 ,0x10 ,0x18 ,0x67 ,0x12 ,0xB0 ,0x3C,
0x00 ,0x01 ,0x66 ,0x06 ,0x43 ,0xE9 ,0x00 ,0xFE ,0x60 ,0xF0 ,0xD2 ,0xC0 ,0xD3 ,0x91 ,0x60 ,0xEA,
0x43 ,0xED ,0x01 ,0x1C ,0x22 ,0x09 ,0x2B ,0x41 ,0x00 ,0x08 ,0x20 ,0x2C ,0x00 ,0x02 ,0x2B ,0x40,
0x00 ,0x0C ,0xD2 ,0x80 ,0x2B ,0x41 ,0x00 ,0x10 ,0x20 ,0x2C ,0x00 ,0x06 ,0x2B ,0x40 ,0x00 ,0x14,
0xD2 ,0x80 ,0x2B ,0x41 ,0x00 ,0x18 ,0x2B ,0x6C ,0x00 ,0x0A ,0x00 ,0x1C ,0x91 ,0xC8 ,0x4E ,0xD1
};

// Scratch buffer for LZSS-compressed output during cart-build. Sized so
// that even a "compressed" output bigger than the input fits without
// triggering a buffer-overflow error (the auto-fallback in -z mode will
// then detect that compression didn't help and ship uncompressed).
unsigned char compress_temp_buf[256 * 1024];

uint32_t parse_bss_parameter(char *p)
{
    if (!p)
    {
        printf("Error: -b expects address in hex, for example -b68000\n");
        exit(-1);
    }
    return strtol(p, NULL, 16);
}

uint32_t parse_init_parameter(char *p)
{
    if (!p || (*p != '0' && *p != '1' && *p != '3' && *p != '5' && *p != '6' && *p != '7'))
    {
        printf("Error: -f expects a value from 0, 1, 3, 5, 6, 7\n");
        exit(-1);
    }

    return 1 << (24 + (*p - '0'));
}

// Convert the input file's mtime to GEMDOS time / date words for the CA_HEADER.
// GEMDOS time = (hours << 11) | (minutes << 5) | (seconds / 2)
// GEMDOS date = ((year - 1980) << 9) | (month << 5) | day
// Years are clamped to the GEMDOS-representable range [1980, 2107]. If stat()
// fails the cart gets 1980-01-01 00:00:00 -- not pretty, but deterministic.
static void gemdos_time_date_from_file(const char *path,
                                       uint16_t *out_time,
                                       uint16_t *out_date)
{
    usm_stat_t st;
    struct tm *tmv;
    time_t mtime = 0;

    if (USM_STAT(path, &st) == 0)
    {
        mtime = st.st_mtime;
    }

    tmv = localtime(&mtime);
    int year   = tmv ? tmv->tm_year + 1900 : 1980;
    int month  = tmv ? tmv->tm_mon + 1     : 1;
    int day    = tmv ? tmv->tm_mday        : 1;
    int hour   = tmv ? tmv->tm_hour        : 0;
    int minute = tmv ? tmv->tm_min         : 0;
    int second = tmv ? tmv->tm_sec         : 0;

    if (year < 1980) { year = 1980; month = 1; day = 1; hour = 0; minute = 0; second = 0; }
    if (year > 2107) { year = 2107; }

    *out_time = (uint16_t)((hour << 11) | (minute << 5) | (second / 2));
    *out_date = (uint16_t)(((year - 1980) << 9) | (month << 5) | day);
}

// LZSS-12-4 compression. See Epic 002 / Story 1 for the format spec.
//
// Stream is a sequence of <= 9-byte blocks. Each block is 1 flag byte
// followed by up to 8 tokens. The flag's bits (MSB-first) classify each
// token: 0 = literal (1 byte); 1 = back-reference (2 bytes, big-endian,
// packed as (offset-1) << 4 | (length-3) with offset in 1..4096 and
// length in 3..18). The decoder is told the expected uncompressed size
// up front, so no EOF marker is needed in the stream itself.
//
// The encoder is greedy longest-match: at each input position scan the
// preceding 4 KB for the longest run that matches. Naive O(N * window)
// search; fast enough for the few-hundred-KB inputs USM handles.
#define LZSS_OFF_BITS    12
#define LZSS_LEN_BITS    4
#define LZSS_WIN_SIZE    (1 << LZSS_OFF_BITS)          // 4096
#define LZSS_MIN_MATCH   3
#define LZSS_MAX_MATCH   (LZSS_MIN_MATCH + (1 << LZSS_LEN_BITS) - 1)  // 18

static int lzss_compress(const unsigned char *src, size_t srclen,
                         unsigned char *dst, size_t dstcap,
                         size_t *out_len)
{
    size_t in = 0;
    size_t out = 0;
    while (in < srclen)
    {
        if (out >= dstcap) return -1;
        size_t flag_pos = out++;
        unsigned char flag = 0;

        for (int bit = 0; bit < 8 && in < srclen; bit++)
        {
            // Find the longest match in [window_start, in).
            size_t window_start = (in > LZSS_WIN_SIZE) ? in - LZSS_WIN_SIZE : 0;
            size_t best_off = 0;
            size_t best_len = 0;
            size_t max_l    = LZSS_MAX_MATCH;
            if (in + max_l > srclen) max_l = srclen - in;

            // Scan latest-first so a tie picks the closest occurrence.
            for (size_t j = in; j > window_start; )
            {
                j--;
                size_t l = 0;
                while (l < max_l && src[j + l] == src[in + l]) l++;
                if (l > best_len)
                {
                    best_len = l;
                    best_off = in - j;
                    if (best_len == LZSS_MAX_MATCH) break;
                }
            }

            if (best_len >= LZSS_MIN_MATCH)
            {
                if (out + 2 > dstcap) return -1;
                uint32_t enc_off = (uint32_t)(best_off - 1);
                uint32_t enc_len = (uint32_t)(best_len - LZSS_MIN_MATCH);
                uint32_t word = (enc_off << LZSS_LEN_BITS) | enc_len;
                dst[out++] = (unsigned char)(word >> 8);
                dst[out++] = (unsigned char)(word & 0xff);
                flag |= (unsigned char)(1 << (7 - bit));
                in += best_len;
            }
            else
            {
                if (out + 1 > dstcap) return -1;
                dst[out++] = src[in++];
            }
        }

        dst[flag_pos] = flag;
    }

    *out_len = out;
    return 0;
}

// LZSS-12-4 decompression. Mirrors the algorithm the 68k stub will run.
// Caller passes the expected uncompressed size (`expected_size`); the
// decoder produces exactly that many output bytes, returning 0 on
// success or non-zero if input is exhausted / a back-reference points
// before the start of output.
static int lzss_decompress(const unsigned char *src, size_t srclen,
                           unsigned char *dst, size_t expected_size)
{
    size_t in = 0;
    size_t out = 0;
    while (out < expected_size)
    {
        if (in >= srclen) return -1;
        unsigned char flag = src[in++];
        for (int bit = 0; bit < 8 && out < expected_size; bit++)
        {
            if (flag & (unsigned char)(1 << (7 - bit)))
            {
                if (in + 2 > srclen) return -1;
                uint32_t word = ((uint32_t)src[in] << 8) | src[in + 1];
                in += 2;
                size_t off = (word >> LZSS_LEN_BITS) + 1;
                size_t len = (word & ((1 << LZSS_LEN_BITS) - 1)) + LZSS_MIN_MATCH;
                if (off > out) return -1;
                if (out + len > expected_size) return -1;
                for (size_t k = 0; k < len; k++)
                {
                    dst[out] = dst[out - off];
                    out++;
                }
            }
            else
            {
                if (in >= srclen) return -1;
                dst[out++] = src[in++];
            }
        }
    }
    return 0;
}

// Quick startup self-test. Runs once per usm invocation, on a small
// hand-rolled buffer that exercises both literal and back-reference
// code paths. Any future regression in the encoder/decoder pair gets
// caught before the user's data is touched.
static int lzss_selftest(void)
{
    static const char test[] =
        "Hello, World! Hello, World! Hello, World!"
        "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    size_t test_len = sizeof(test) - 1;
    unsigned char comp[256];
    unsigned char deco[sizeof(test)];
    size_t comp_len = 0;
    if (lzss_compress((const unsigned char *)test, test_len,
                      comp, sizeof(comp), &comp_len) != 0) return -1;
    if (lzss_decompress(comp, comp_len, deco, test_len) != 0) return -2;
    if (memcmp(test, deco, test_len) != 0) return -3;
    return 0;
}

// Hidden debug command: round-trip a file through lzss_compress() and
// lzss_decompress(), print the ratio, exit 0 on success / 1 on any
// mismatch. Not advertised in --help; used by tests/ to validate the
// LZSS implementation before -z compression is wired into the cart
// build (Story 3). Usage: usm -T <file>.
static int lzss_roundtrip_file(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) { printf("cannot open %s\n", path); return 1; }
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (sz < 0) { fclose(fp); printf("ftell failed on %s\n", path); return 1; }
    unsigned char *orig = malloc((size_t)sz);
    unsigned char *comp = malloc((size_t)sz * 2 + 64); // safe upper bound
    unsigned char *deco = malloc((size_t)sz);
    if (!orig || !comp || !deco) { printf("oom\n"); return 1; }
    if (fread(orig, 1, (size_t)sz, fp) != (size_t)sz) { fclose(fp); printf("read failed\n"); return 1; }
    fclose(fp);
    size_t comp_len = 0;
    if (lzss_compress(orig, (size_t)sz, comp, (size_t)sz * 2 + 64, &comp_len) != 0)
    { printf("%s: compress failed\n", path); return 1; }
    if (lzss_decompress(comp, comp_len, deco, (size_t)sz) != 0)
    { printf("%s: decompress failed\n", path); return 1; }
    if (memcmp(orig, deco, (size_t)sz) != 0)
    { printf("%s: round-trip MISMATCH\n", path); return 1; }
    double ratio = (double)comp_len / (double)sz * 100.0;
    printf("%s: %ld -> %zu bytes (%.1f%%) OK\n", path, sz, comp_len, ratio);
    free(orig); free(comp); free(deco);
    return 0;
}

int main(int argc, char **argv)
{
    int diagnostic = 0;
    unsigned char *cart_start = cart;
    int cart_current_offset = 0;
    int steem_cart = 0;
    int i;

    // Always self-test the LZSS encoder + decoder on a hardcoded buffer
    // before doing anything else. Catches regressions in the round-trip
    // pair before the user's data is touched.
    if (lzss_selftest() != 0)
    {
        printf("Internal error: LZSS self-test failed - exiting\n");
        return -1;
    }

    // Hidden debug command for Story 2 testing: usm -T <file>
    // Compresses the file in memory, decompresses, byte-compares.
    if (argc == 3 && argv[1][0] == '-' && argv[1][1] == 'T' && argv[1][2] == '\0')
    {
        return lzss_roundtrip_file(argv[2]);
    }

    if (argc < 3)
    {
        printf("Usage: %s [-] [-s] -c [-bX] [-fY] image_filename <list of programs to add with optional -b and -f>\n"
            "pass -s to create a steem engine compatible cart image file\n"
            "pass -d to create a diagnostic cart image file\n"
            "pass -c to switch to the classic way of adding the application to the ROM"
            "pass -bAABBCC to set BSS to $AABBCC (only when -c is active, default: $20000)\n"
            "pass -fX to set the INIT flag. X can be:\n"
            "0       Execute prior to display memory and interrupt vector initialization.\n"
            "1       Execute just before GEMDOS is initialized.\n"
            "3       Execute prior to boot disk.\n"
            "5       Application is a Desk Accessory.\n"
            "6       Application is not a GEM application.\n"
            "7       Application needs parameters.\n"
            "-b and -f can be passed at the beginning of argument list where they will\n"
            "have global effect, or after each filename where they'll only apply to the current file.\n"
            "Default value for -b is $20000 and for -f is 0.\n", argv[0]);
        return -1;
    }

    argv++;
    argc--;

    uint32_t global_bss_hardcoded_address = 0x20000;
    uint32_t global_init_flag = 0;
    int classic_way_of_adding_programs_to_rom = 0;
    int global_compress = 0;
    const uint32_t diagnostic_magic = 0xfa52235f;
    const uint32_t cart_magic = 0xABCDEF42;

    while (argc && **argv == '-')
    {
        if ((*argv)[1] == 's')
        {
            steem_cart = 1;
        }
        else if ((*argv)[1] == 'd')
        {
            // Diagnostic cartridges are executed almost immediately after a system reset.
            // The OS uses a 680x0 JMP instruction to begin execution at address 0xFA0004
            // after having set the Interrupt Priority Level (IPL) to 7, entering supervisor mode,
            // and executing a RESET instruction to reset external hardware devices.
            diagnostic = 1;
        }
        else if ((*argv)[1] == 'b')
        {
            global_bss_hardcoded_address = parse_bss_parameter(&argv[0][2]);
        }
        else if ((*argv)[1] == 'f')
        {
            global_init_flag = parse_init_parameter(&argv[0][2]);
        }
        else if ((*argv)[1] == 'z')
        {
            // -z = compress PRG payload(s) with LZSS-12-4. Auto-falls
            // back to the uncompressed path per program if the compressed
            // entry isn't smaller (see the per-file loop below).
            global_compress = 1;
        }
        else if ((*argv)[1] == 'c')
        {
            classic_way_of_adding_programs_to_rom = 1;
        }
        else
        {
            printf("Invalid flag passed '%s' - exiting\n", *argv);
            return -1;
        }
        argv++;
        argc--;
    }

    if (argc < 1)
    {
        printf("Missing image filename - exiting\n");
        return -1;
    }
    // -z is incompatible with -c (classic mode runs the program directly
    // from ROM -- nowhere to decompress to) and with -d (diagnostic carts
    // execute with no TOS context, but the decompressor needs Malloc).
    // Reject as soon as both flags are visible globally; per-file overrides
    // are re-checked inside the per-program loop below.
    if (global_compress && classic_way_of_adding_programs_to_rom)
    {
        printf("-z (compress) and -c (classic mode) are incompatible - exiting\n");
        return -1;
    }
    if (global_compress && diagnostic)
    {
        printf("-z (compress) and -d (diagnostic) are incompatible - exiting\n");
        return -1;
    }

    char *output_filename = *argv;
    argv++;
    argc--;

    unsigned char *fill = cart;
    for (i = 0; i < 128 * 1024 / 4; i++)
    {
        *fill++ = 'U';
        *fill++ = 'S';
        *fill++ = 'M';
        *fill++ = '!';
    }

    uint32_t magic_at_zero = diagnostic ? diagnostic_magic : cart_magic;
    *(uint32_t *)&cart_start[cart_current_offset] = BYTESWAP_LONG(magic_at_zero);
    cart_current_offset += 4;

    uint32_t *last_ca_next = 0; // It had better be initialised when we exit the loop below
    int number_of_programs_processed = 0;

    while (argc)
    {
        uint32_t bss_current_file = 0;
        uint32_t init_current_file = global_init_flag;
        int compress_current_file = global_compress;
        size_t compressed_size = 0;
        int use_compressed = 0;

        FILE *f = fopen(*argv, "rb");
        if (!f)
        {
            printf("File %s not found - exiting\n", *argv);
            return -1;
        }

        fseek(f, 0, SEEK_END);
        long file_size = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (file_size < 0)
        {
            fclose(f);
            printf("Could not determine size of %s - exiting\n", *argv);
            return -1;
        }
        if ((size_t)file_size < sizeof(PRG_HEADER))
        {
            fclose(f);
            printf("File %s is too small to be a PRG (%ld bytes, need at least %zu) - exiting\n",
                   *argv, file_size, sizeof(PRG_HEADER));
            return -1;
        }
        if (file_size > 128 * 1024)
        {
            // Without any further checks, this will not fit. RIP
            fclose(f);
            printf("File %s will not fit in image - exiting\n", *argv);
            return -1;
        }

        size_t ret = fread(prg_temp_buf, file_size, 1, f);
        fclose(f);
        if (!ret)
        {
            printf("Failed to read file %s - exiting\n", *argv);
            return -1;
        }

        uint32_t prg_header_size = sizeof(PRG_HEADER);
        uint32_t program_size;
        PRG_HEADER *ph = (PRG_HEADER *)prg_temp_buf;
        uint16_t magic = BYTESWAP_WORD(ph->PRG_magic);
        if (magic != 0x601a)
        {
            printf("File %s is not a PRG (magic 0x%04x, expected 0x601a) - exiting\n", *argv, magic);
            return -1;
        }

        char *s_filename = *argv;
        char *file_path = s_filename; // s_filename is consumed by the 8.3 loop below; keep an unmutated copy for late error messages.

        // CA_FILENAME is 8.3 GEMDOS; strip any directory prefix from argv so a
        // path like "tests/binaries/MONST2.PRG" lands as "MONST2.PRG" instead
        // of the previous "TESTS/BI.PRG" (the 8.3 loop didn't treat '/' or '\\'
        // as delimiters).
        for (char *p = file_path; *p; p++)
        {
            if (*p == '/' || *p == '\\')
            {
                s_filename = p + 1;
            }
        }

        argv++;
        argc--;

        while (argc && **argv == '-')
        {
            if ((*argv)[1] == 'b')
            {
                // Current program has a companion '-b', so we'll override the default BSS address
                bss_current_file = parse_bss_parameter(&argv[0][2]);
                argv++;
                argc--;
            }
            else if ((*argv)[1] == 'f')
            {
                init_current_file = parse_init_parameter(&argv[0][2]);
                argv++;
                argc--;
            }
            else if ((*argv)[1] == 'c')
            {
                classic_way_of_adding_programs_to_rom = 1;
                argv++;
                argc--;
            }
            else if ((*argv)[1] == 'z')
            {
                // Per-file -z: enable compression for the just-read PRG only.
                compress_current_file = 1;
                argv++;
                argc--;
            }
            else
            {
                printf("Invalid flag passed '%s' - exiting\n", *argv);
                return -1;
            }
        }

        // Re-check the -z conflicts at per-file granularity, in case the
        // per-file flag loop added -c or the cart is diagnostic.
        if (compress_current_file && classic_way_of_adding_programs_to_rom)
        {
            printf("File %s: -z (compress) and -c (classic mode) are incompatible - exiting\n", s_filename);
            return -1;
        }
        if (compress_current_file && diagnostic)
        {
            printf("File %s: -z (compress) and -d (diagnostic) are incompatible - exiting\n", s_filename);
            return -1;
        }

        if (diagnostic && !classic_way_of_adding_programs_to_rom)
        {
            // Diagnostic carts auto-execute from $FA0004 with no basepage, but
            // the default-mode stub loader assumes a TOS Pexec entry with the
            // basepage on the stack -- the combination produces a cart that
            // crashes immediately on insertion. Force -c when -d is in effect.
            printf("-d (diagnostic cart) requires -c (classic mode); the default stub loader needs a TOS basepage that diagnostic carts don't provide - exiting\n");
            return -1;
        }

        program_size = (BYTESWAP_LONG(ph->PRG_tsize) + BYTESWAP_LONG(ph->PRG_dsize) + 1) & 0xfffffffe; // align to 2 bytes

        // Compression attempt: try LZSS-12-4 against the whole PRG file. Use
        // the compressed entry only if it's actually smaller (decompressor
        // stub + uncompressed_size LONG + compressed bytes vs. uncompressed
        // stub + verbatim file). Already-packed PRGs typically grow under
        // LZSS; the auto-fallback below ships those uncompressed.
        if (compress_current_file && !classic_way_of_adding_programs_to_rom)
        {
            size_t comp_len = 0;
            int rc = lzss_compress(prg_temp_buf, (size_t)file_size,
                                   compress_temp_buf, sizeof(compress_temp_buf),
                                   &comp_len);
            uint32_t uncomp_footprint = (uint32_t)sizeof(prg_loader) + (uint32_t)file_size;
            uint32_t comp_footprint   = (uint32_t)sizeof(prg_loader_compressed) + 4u + (uint32_t)comp_len;
            if (rc == 0 && comp_footprint < uncomp_footprint)
            {
                compressed_size = comp_len;
                use_compressed = 1;
                printf("      + %s: compressed %ld -> %zu (%.1f%%)\n",
                       s_filename, file_size, comp_len,
                       (double)comp_len / (double)file_size * 100.0);
            }
            else
            {
                // Report the *entry* footprint ratio rather than the data
                // ratio. For tiny PRGs the data may shrink (e.g. 88%) but
                // the larger compressed stub still pushes the total entry
                // over the uncompressed baseline; the entry ratio captures
                // both stub overhead and data growth in one number.
                double entry_ratio = rc == 0
                    ? (double)comp_footprint / (double)uncomp_footprint * 100.0
                    : 200.0;
                printf("      + %s: no savings (%.0f%% entry), shipping uncompressed\n",
                       s_filename, entry_ratio);
            }
        }

        // Per-program footprint in the cart: CA_HEADER (if not diagnostic) plus
        // the bytes we actually write -- TEXT+DATA only in classic mode;
        // compressed stub + uncompressed_size LONG + compressed bytes in
        // compressed default mode; uncompressed stub + verbatim PRG otherwise.
        // Drives both the overflow check and CA_NEXT so the chain actually
        // points at the next CA_HEADER.
        uint32_t entry_header_size = diagnostic ? 0 : (uint32_t)sizeof(CA_HEADER);
        uint32_t payload_in_cart;
        if (classic_way_of_adding_programs_to_rom)
        {
            payload_in_cart = program_size;
        }
        else if (use_compressed)
        {
            payload_in_cart = (uint32_t)sizeof(prg_loader_compressed) + 4u + (uint32_t)compressed_size;
        }
        else
        {
            payload_in_cart = (uint32_t)sizeof(prg_loader) + (uint32_t)file_size;
        }
        uint32_t entry_total_size = entry_header_size + payload_in_cart;
        // Round each entry's footprint up to a word boundary so the next
        // CA_HEADER never lands on an odd address. TOS walks the cart's
        // CA_NEXT chain at boot using long-word reads (TST.L (A0)), which
        // raise a 68000 Address Error on an odd pointer and crash the
        // machine with a line of bombs before GEM is up.
        uint32_t entry_total_size_padded = (entry_total_size + 1u) & ~1u;

        if (entry_total_size_padded > (uint32_t)(128 * 1024) - cart_current_offset)
        {
            printf("File %s will not fit in image - exiting\n", s_filename);
            return -1;
        }

        // Write header
        if (!diagnostic)
        {
            CA_HEADER *h = (CA_HEADER *)&cart_start[cart_current_offset];
            last_ca_next = &h->CA_NEXT;
            h->CA_NEXT = BYTESWAP_LONG(0xfa0000 + cart_current_offset + entry_total_size_padded);
            // If no init flag was requested, leave CA_INIT entirely zero
            // (entire LONG = 0). Some TOS variants treat any non-zero
            // CA_INIT value as a callable address even when the high-byte
            // "init flags" are clear, and would auto-run our stub at boot
            // before GEMDOS is ready. The cart program remains launchable
            // via CA_RUN from the c: desktop drive.
            h->CA_INIT = init_current_file
                ? BYTESWAP_LONG(0xfa0000 + cart_current_offset + sizeof(CA_HEADER) + init_current_file)
                : 0;
            h->CA_RUN = BYTESWAP_LONG(0xfa0000 + cart_current_offset + sizeof(CA_HEADER));
            uint16_t ca_time_val, ca_date_val;
            gemdos_time_date_from_file(file_path, &ca_time_val, &ca_date_val);
            h->CA_TIME = BYTESWAP_WORD(ca_time_val);
            h->CA_DATE = BYTESWAP_WORD(ca_date_val);
            h->CA_SIZE = BYTESWAP_LONG(payload_in_cart);
            memset(h->CA_FILENAME, 0, 14);
            char *d_filename = h->CA_FILENAME;
            for (i = 0; i < 8; i++)
            {
                // Copy up to 8 characters, with uppercase
                char c = toupper(*s_filename);
                if (!c || c == '.')
                {
                    break;
                }
                *d_filename++ = c;
                s_filename++;
            }
            while (*s_filename != '.' && *s_filename)
            {
                // Truncate the rest of the characters of the filename
                s_filename++;
            }
            for (i = 0; i < 4; i++)
            {
                // Copy dot character, plus up to 3 characters extension, with uppercase
                char c = toupper(*s_filename);
                if (!c)
                {
                    break;
                }
                *d_filename++ = c;
                s_filename++;
            }
            //strcpy(h->CA_FILENAME, *argv);    // TODO: Truncate to 8.3 if needed, convert to uppercase

            cart_current_offset += sizeof(CA_HEADER);
        }

        if (classic_way_of_adding_programs_to_rom)
        {
            // Copy program TEXT and DATA sections without header, relocate TEXT and DATA sections
            // to map to ROM space, relocate BSS to hardcoded RAM area (defined by -f)
            memcpy(&cart_start[cart_current_offset], prg_temp_buf + prg_header_size, program_size);

            // Relocate program
            if (!bss_current_file)
            {
                // use global default if bss address not specified for this program
                bss_current_file = global_bss_hardcoded_address;
            }
            if (!ph->ABSFLAG)
            {
            uint32_t program_start_address = 0xfa0000 + cart_current_offset;
                unsigned char *current_relocation = &cart_start[cart_current_offset];
                unsigned char *prog_end = current_relocation + program_size;
                unsigned char *reloc = &prg_temp_buf[prg_header_size + BYTESWAP_LONG(ph->PRG_tsize) + BYTESWAP_LONG(ph->PRG_dsize) + BYTESWAP_LONG(ph->PRG_ssize)];
                unsigned char *reloc_end = (unsigned char *)&prg_temp_buf[file_size];
                if (reloc + 4 > reloc_end)
                {
                    printf("Relocation table in %s extends past end of file - exiting\n", file_path);
                    return -1;
                }
            uint32_t offset = BYTESWAP_LONG(*(uint32_t *)reloc);
                reloc += 4;
                // First LONG of 0 here is the PRG-spec alt encoding for "no fixups";
                // skip the walker entirely in that case (the old `for(;;)` body would
                // have fixed up offset 0 -- the first bytes of TEXT -- by mistake).
                while (offset != 0)
                {
                    if (offset == 1)
                    {
                        current_relocation += 254;
                    }
                    else
                    {
                        current_relocation += offset;
                        if (current_relocation + 4 > prog_end)
                        {
                            printf("Relocation in %s points past end of program - exiting\n", file_path);
                            return -1;
                        }
                    uint32_t off = BYTESWAP_LONG(*(uint32_t *)current_relocation);
                        if (off < program_size)
                        {
                            // It's within the text/data section, relocate as usual
                            *(uint32_t *)current_relocation = BYTESWAP_LONG(program_start_address + off);
                        }
                        else
                        {
                            // It's BSS area, point it to the hardcoded BSS address
                            *(uint32_t *)current_relocation = BYTESWAP_LONG(off - program_size + bss_current_file);
                        }
                    }
                    if (reloc >= reloc_end)
                    {
                        printf("Relocation table in %s missing terminator - exiting\n", file_path);
                        return -1;
                    }
                    if (!*reloc) break;
                    offset = *reloc++;
                }
            }

            cart_current_offset += program_size;
        }
        else if (use_compressed)
        {
            // Compressed default mode. Cart layout for this entry:
            //   [ compressed stub ][ uncompressed_size LONG ][ compressed bytes ]
            // The stub's `lea prg_payload(pc), a4` lands on the LONG, then
            // advances into the compressed bytes after reading it. See
            // prg_loader_compressed.s.
            memcpy(&cart_start[cart_current_offset], prg_loader_compressed,
                   sizeof(prg_loader_compressed));
            cart_current_offset += sizeof(prg_loader_compressed);

            uint32_t usz_be = BYTESWAP_LONG((uint32_t)file_size);
            memcpy(&cart_start[cart_current_offset], &usz_be, 4);
            cart_current_offset += 4;

            memcpy(&cart_start[cart_current_offset], compress_temp_buf, compressed_size);
            cart_current_offset += compressed_size;
        }
        else
        {
            // Copy the stub loader, then copy the PRG file verbatim immediately
            // after it. The stub locates its payload via PC-relative addressing
            // (lea prg_payload(pc), a4 inside the stub), so the same stub bytes
            // work for every program in a multi-program cart without any
            // build-time patching. The stub itself does Malloc(largest free)
            // for a fresh TPA, builds the basepage, copies + relocates the PRG
            // into RAM, zero-fills BSS, then JMPs to the program. See
            // prg_loader.s for the asm source.
            memcpy(&cart_start[cart_current_offset], prg_loader, sizeof(prg_loader));
            cart_current_offset += sizeof(prg_loader);

            memcpy(&cart_start[cart_current_offset], prg_temp_buf, file_size);
            cart_current_offset += file_size;
        }

        // Word-align the next entry's start (see comment at entry_total_size_padded
        // above). The skipped byte, if any, keeps its USM!-fill value; nothing reads it.
        if (cart_current_offset & 1u)
        {
            cart_current_offset++;
        }

        number_of_programs_processed++;

        if (diagnostic && argc)
        {
            // If we reached this point, the user told us to create a diagnostic cart
            // that has more than one filename, and that's a no go. Complain and exit
            printf("We were asked to create a diagnostic cart and were given\n"
                   "more than one program. This is not supported - exiting\n");
            return -1;
        }
    }

    if (!number_of_programs_processed)
    {
        printf("No programs supplied - exiting.\n");
        return -1;
    }

    if (!diagnostic)
    {
        *last_ca_next = 0; // Last program in the chain needs to have CA_NEXT=0
    }

    // Write out the file
    FILE *f = fopen(output_filename, "wb");
    if (!f)
    {
        printf("Could not create image file %s  - exiting\n", output_filename);
        return -1;
    }
    int write_ok = 1;
    if (steem_cart)
    {
        // Write an extra 0.l at the start of the file
        steem_cart = 0;
        if (fwrite(&steem_cart, 4, 1, f) != 1) write_ok = 0;
    }
    if (write_ok && fwrite(cart_start, 128 * 1024, 1, f) != 1) write_ok = 0;
    if (fclose(f) != 0) write_ok = 0;
    if (!write_ok)
    {
        printf("Failed writing image file %s - exiting\n", output_filename);
        return -1;
    }

    return 0;
}
