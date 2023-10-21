#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdint.h>

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

int main(int argc, char **argv)
{
    int diagnostic = 0;
    char *cart_start = cart;
    int cart_current_offset = 0;
    int steem_cart = 0;
    int i;

    if (argc < 3)
    {
        printf("Usage: %s [s] image_filename <list_of_programs_to_add>\n"
            "pass s to create a steem engine compatible cart image file\n"
            "For now, use upper case filenames\n", argv[0]);
        return -1;
    }

    // Eat the program filename argument
    argv++;
    argc--;

    if (*argv[0] == 's')
    {
        steem_cart = 1;
        argv++;
        argc--;
    }
    else if (*argv[0] == 'd')
    {
        // Diagnostic cartridges are executed almost immediately after a system reset.
        // The OS uses a 680x0 JMP instruction to begin execution at address 0xFA0004
        // after having set the Interrupt Priority Level (IPL) to 7, entering supervisor mode, 
        // and executing a RESET instruction to reset external hardware devices.
        unsigned long diagnostic_magic = 0xFA52255F;
        printf("Diagnostic cart not yet supported - exiting\n");
        return -1;
    }


    if (diagnostic)
    {
        // ...
    }

    unsigned char *fill = cart;
    for (i = 0; i < 128 * 1024 / 4; i++)
    {
        *fill++ = 'G';
        *fill++ = 'G';
        *fill++ = 'N';
        *fill++ = '!';
    }
    
    *(unsigned long *)&cart_start[cart_current_offset] = BYTESWAP_LONG(0xABCDEF42);
    cart_current_offset += 4;
    unsigned long bss_hardcoded_address = 0x20000; // TODO UI

    for (i = 1; i < argc; i++)
    {
        FILE *f = fopen(argv[i], "rb");
        if (!f)
        {
            printf("File %s not found - exiting\n",argv[i]);
            return -1;
        }

        fseek(f, 0, SEEK_END);
        long file_size = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (file_size > 128 * 1024)
        {
            // Without any further checks, this will not fit. RIP
            fclose(f);            
            printf("File %s will not fit in image - exiting\n", argv[i]);
            return -1;
        }

        fread(prg_temp_buf, file_size, 1, f);   // TODO check errors etc
        fclose(f);

        long header_size = sizeof(PRG_HEADER);
        unsigned long program_size;
        PRG_HEADER *ph = (PRG_HEADER *)prg_temp_buf;
        // TODO: sanity checks for values here
        if (BYTESWAP_WORD(ph->PRG_magic) != 0x601a)
        {
            // TODO
        }

        program_size = (BYTESWAP_LONG(ph->PRG_tsize) + BYTESWAP_LONG(ph->PRG_dsize) + 1) & 0xfffffffe; // align to 2 bytes

        if (program_size > 128 * 1024 - cart_current_offset - sizeof(CA_HEADER))
        {
            fclose(f);
            printf("File %s will not fit in image - exiting\n", argv[i]);
            return -1;
        }

        // Write header
        unsigned long init_flags = 0 << 24; // TODO UI
        CA_HEADER *h = (CA_HEADER *)&cart_start[cart_current_offset];
        h->CA_NEXT = 0;
        if (i < argc - 1)
        {
            h->CA_NEXT = BYTESWAP_LONG(0xfa0000 + cart_current_offset + sizeof(CA_HEADER) + program_size);
        }
        h->CA_INIT = BYTESWAP_LONG(0xfa0000 + cart_current_offset + sizeof(CA_HEADER) + init_flags);
        h->CA_RUN = BYTESWAP_LONG(0xfa0000 + cart_current_offset + sizeof(CA_HEADER));
        h->CA_TIME = 0; //TODO figure out from file info (if we really care)
        h->CA_DATE = 0; //TODO figure out from file info (if we really care)
        h->CA_SIZE = BYTESWAP_LONG(program_size);
        memset(h->CA_FILENAME, 0, 14);
        strcpy(h->CA_FILENAME, argv[i]);    // TODO: Truncate to 8.3 if needed, convert to uppercase

        cart_current_offset += sizeof(CA_HEADER);

        memcpy(&cart_start[cart_current_offset], prg_temp_buf + header_size, program_size);

        // Relocate program
        if (!ph->ABSFLAG)
        {
            unsigned long program_start_address = 0xfa0000 + cart_current_offset;
            unsigned char *current_relocation = &cart_start[cart_current_offset];
            unsigned char *reloc = &prg_temp_buf[header_size + BYTESWAP_LONG(ph->PRG_tsize) + BYTESWAP_LONG(ph->PRG_dsize) + BYTESWAP_LONG(ph->PRG_ssize)]; // TODO: Sanity check values
            unsigned long offset = BYTESWAP_LONG(*(uint32_t *)reloc);
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
                    unsigned long off = BYTESWAP_LONG(*(uint32_t *)current_relocation);
                    if (off < program_size)
                    {
                        // It's within the text/data section, relocate as usual
                        *(uint32_t *)current_relocation = BYTESWAP_LONG(program_start_address + off);
                    }
                    else
                    {
                        // It's BSS area, point it to the hardcoded BSS address
                        *(uint32_t *)current_relocation = BYTESWAP_LONG(off - program_size + bss_hardcoded_address);
                    }
                }
                if (!*reloc) break;
                offset = *reloc++;
            }
        }

        cart_current_offset += program_size;

    }

    FILE *f = fopen(argv[0], "wb");
    if (!f)
    {
        printf("Could not create image file %s  - exiting\n", argv[0]);
        return -1;
    }
    if (steem_cart)
    {
        // Write an extra 0.l at the start of the file
        steem_cart = 0;
        fwrite(&steem_cart, 4, 1, f);
    }
    fwrite(cart_start, 128*1024, 1, f);
    fclose(f);

    return 0;
}