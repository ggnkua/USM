#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

char cart[1024 * 128];
char prg_temp_buf[1024 * 128];

#if defined (_WIN32)
#pragma pack(2)
#endif
#if defined(__linux__) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__APPLE__)
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
    int8_t      CA_FILENAME[14];    // NULL terminated ASCII filename in standard GEMDOS 8+3 format.
} CA_HEADER;

typedef struct
{
    uint8_t     PRG_magic;  // This WORD contains the magic value (0x601A).
    uint32_t    PRG_tsize;  // This LONG contains the size of the TEXT segment in bytes.
    uint32_t    PRG_dsize;  // This LONG contains the size of the DATA segment in bytes.
    uint32_t    PRG_bsize;  // This LONG contains the size of the BSS segment in bytes.
    uint32_t    PRG_ssize;  // This LONG contains the size of the symbol table in bytes.
    uint32_t    PRG_res1;   // This LONG is unused and is currently reserved.
    uint32_t    PRGFLAGS;   // This LONG contains flags which define certain process characteristics (as defined below).
    uint8_t     ABSFLAG;    // This WORD flag should be non-zero to indicate that the program has no fixups or 0 to indicate it does.Since some versions of TOS handle files with this value being non-zero incorrectly, it is better to represent a program having no fixups with 0 here and placing a 0 longword as the fixup offset.
} PRG_HEADER;
#if defined(__linux__) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__APPLE__)
#pragma pack(pop)
#endif

#if defined(__linux__) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__APPLE__) || defined (_WIN32)
#define BYTESWAP_LONG(x) (((x&0xff000000)>>24)|((x&0x00ff0000)>>8)|((x&0x0000ff00)<<8)|((x&0x000000ff)<<24))
#define BYTESWAP_WORD(x) (((x&0xff00)>>8)|((x&0x00ff)<<8))
#else
#define BYTESWAP_LONG(x) x
#define BYTESWAP_WORD(x) x
#endif

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

int main(int argc, char **argv)
{
    int diagnostic = 0;
    char *cart_start = cart;
    int cart_current_offset = 0;
    int steem_cart = 0;
    int i;

    if (argc < 3)
    {
        printf("Usage: %s [-s] [-bX] [-fY] image_filename <list of programs to add with optional -b and -f>\n"
            "pass -s to create a steem engine compatible cart image file\n"
            "pass -d to create a diagnostic cart image file\n"
            "pass -bAABBCC to set BSS to $AABBCC (default: $20000)\n"
            "pass -fX to set the INIT flag. X can be:\n"
            "0       Execute prior to display memory and interrupt vector initialization.\n"
            "1       Execute just before GEMDOS is initialized.\n"
            "3       Execute prior to boot disk.\n"
            "5       Application is a Desk Accessory.\n"
            "6       Application is not a GEM application.\n"
            "7       Application needs parameters.\n"
            "-b and -f can be passed at the beginning of argument list where they will\n"
            "have global effect, or after each filename where they'll only apply to the current file.\n"
            "Default value for -b is $20000 and for -f is nothing.\n", argv[0]);
        return -1;
    }

    argv++;
    argc--;

    uint32_t gloal_bss_hardcoded_address = 0x20000;
    uint32_t global_init_flag = 0;
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
            gloal_bss_hardcoded_address = parse_bss_parameter(&argv[0][2]);
        }
        else if ((*argv)[1] == 'f')
        {
            global_init_flag = parse_init_parameter(&argv[0][2]);
        }
        else
        {
            printf("Invalid flag passed '%s' - exiting\n", *argv);
            return -1;
        }
        argv++;
        argc--;
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

    *(uint32_t *)&cart_start[cart_current_offset] = BYTESWAP_LONG(cart_magic);
    if (diagnostic)
    {
        *(uint32_t *)&cart_start[cart_current_offset] = BYTESWAP_LONG(diagnostic_magic);
    }
    cart_current_offset += 4;

    uint32_t *last_ca_next = 0; // It had better be initialised when we exit the loop below
    int number_of_programs_processed = 0;

    while (argc)
    {
        uint32_t bss_current_file = 0;
        uint32_t init_current_file = global_init_flag;

        FILE *f = fopen(*argv, "rb");
        if (!f)
        {
            printf("File %s not found - exiting\n", *argv);
            return -1;
        }

        fseek(f, 0, SEEK_END);
        long file_size = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (file_size > 128 * 1024)
        {
            // Without any further checks, this will not fit. RIP
            fclose(f);
            printf("File %s will not fit in image - exiting\n", *argv);
            return -1;
        }

        size_t ret = fread(prg_temp_buf, file_size, 1, f);   // TODO check errors etc
        fclose(f);
        if (!ret)
        {
            printf("Failed to read file %s - exiting", *argv);
        }

        uint32_t prg_header_size = sizeof(PRG_HEADER);
        uint32_t program_size;
        PRG_HEADER *ph = (PRG_HEADER *)prg_temp_buf;
        // TODO: sanity checks for values here
        if (BYTESWAP_WORD(ph->PRG_magic) != 0x601a)
        {
            // TODO
        }

        program_size = (BYTESWAP_LONG(ph->PRG_tsize) + BYTESWAP_LONG(ph->PRG_dsize) + 1) & 0xfffffffe; // align to 2 bytes

        uint32_t entry_header_size = 0;
        if (!diagnostic)
        {
            entry_header_size = sizeof(CA_HEADER);
        }
        if (program_size > 128 * 1024 - cart_current_offset - entry_header_size)
        {
            fclose(f);
            printf("File %s will not fit in image - exiting\n", *argv);
            return -1;
        }
        char *s_filename = *argv;

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
            else
            {
                printf("Invalid flag passed '%s' - exiting\n", *argv);
                return -1;
            }
        }

        // Write header
        if (!diagnostic)
        {
            CA_HEADER *h = (CA_HEADER *)&cart_start[cart_current_offset];
            last_ca_next = &h->CA_NEXT;
            h->CA_NEXT = BYTESWAP_LONG(0xfa0000 + cart_current_offset + sizeof(CA_HEADER) + program_size);
            h->CA_INIT = BYTESWAP_LONG(0xfa0000 + cart_current_offset + sizeof(CA_HEADER) + init_current_file);
            h->CA_RUN = BYTESWAP_LONG(0xfa0000 + cart_current_offset + sizeof(CA_HEADER));
            h->CA_TIME = 0; //TODO figure out from file info (if we really care)
            h->CA_DATE = 0; //TODO figure out from file info (if we really care)
            h->CA_SIZE = BYTESWAP_LONG(program_size);
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

        memcpy(&cart_start[cart_current_offset], prg_temp_buf + prg_header_size, program_size);

        // Relocate program
        if (!bss_current_file)
        {
            // use global default if bss address not specified for this program
            bss_current_file = gloal_bss_hardcoded_address;
        }
        if (!ph->ABSFLAG)
        {
            uint32_t program_start_address = 0xfa0000 + cart_current_offset;
            unsigned char *current_relocation = &cart_start[cart_current_offset];
            unsigned char *reloc = &prg_temp_buf[prg_header_size + BYTESWAP_LONG(ph->PRG_tsize) + BYTESWAP_LONG(ph->PRG_dsize) + BYTESWAP_LONG(ph->PRG_ssize)]; // TODO: Sanity check values
            uint32_t offset = BYTESWAP_LONG(*(uint32_t *)reloc);
            reloc += 4;
            for (;;)
            {
                if (offset == 1)
                {
                    current_relocation += 254;
                }
                else
                {
                    current_relocation += offset;
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
                if (!*reloc) break;
                offset = *reloc++;
            }
        }

        cart_current_offset += program_size;
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
    if (steem_cart)
    {
        // Write an extra 0.l at the start of the file
        steem_cart = 0;
        fwrite(&steem_cart, 4, 1, f);
    }
    fwrite(cart_start, 128 * 1024, 1, f);
    fclose(f);

    return 0;
}