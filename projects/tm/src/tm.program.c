/// @file tm.program.c

#include <tm.program.h>

/* Public Functions ***********************************************************/

tm_program_t* tm_create_program (const char* p_filename)
{
    tm_program_t* l_program = tm_calloc(1, tm_program_t);
    tm_expect_p(l_program != nullptr, "tm: failed to allocate memory for program structure!\n");

    if (tm_init_program(l_program, p_filename) == false)
    {
        tm_destroy_program(l_program);
        return nullptr;
    }

    return l_program;
}

bool tm_init_program (tm_program_t* p_program, const char* p_filename)
{
    // Validate input parameters.
    tm_expect(p_program != nullptr, "tm: program structure is null!\n");
    tm_expect(p_filename != nullptr, "tm: program filename is null!\n");
    tm_expect(p_filename[0] != '\0', "tm: program filename is empty!\n");

    // Attempt to open the program file.
    FILE* l_file = fopen(p_filename, "rb");
    if (l_file == nullptr)
    {
        tm_perrorf("tm: failed to open program file '%s'", p_filename);
        return false;
    }

    // Attempt to determine and validate the size of the program file.
    fseek(l_file, 0, SEEK_END);
    long l_rom_size = ftell(l_file);
    if (l_rom_size < 0)
    {
        tm_perrorf("tm: failed to determine size of program file '%s'", p_filename);
        fclose(l_file);
        return false;
    }
    else if (l_rom_size < TM_ROM_MINIMUM_SIZE)
    {
        tm_errorf("tm: program file '%s' is too small!\n", p_filename);
        tm_errorf("tm:   minimum size is %u bytes, but file is %ld bytes.\n", TM_ROM_MINIMUM_SIZE, l_rom_size);
        fclose(l_file);
        return false;
    }
    else if (l_rom_size > TM_ROM_SIZE)
    {
        tm_errorf("tm: program file '%s' is too large!\n", p_filename);
        tm_errorf("tm:   maximum size is %u bytes, but file is %ld bytes.\n", TM_ROM_SIZE, l_rom_size);
        fclose(l_file);
        return false;
    }
    else
    {
        p_program->m_rom_size = (size_t) l_rom_size;
        rewind(l_file);
    }

    // Attempt to allocate memory for the program ROM.
    p_program->m_rom = tm_malloc(p_program->m_rom_size, byte_t);
    if (p_program->m_rom == nullptr)
    {
        tm_perrorf("tm: failed to allocate memory for program rom");
        fclose(l_file);
        return false;
    }
    
    // Attempt to read the program ROM from the file.
    size_t read_size = fread(p_program->m_rom, sizeof(byte_t), p_program->m_rom_size, l_file);
    if (read_size != p_program->m_rom_size)
    {
        tm_perrorf("tm: failed to read program file '%s'", p_filename);
        tm_free(p_program->m_rom);
        fclose(l_file);
        return false;
    }

    // Validate the magic number from the ROM.
    if (
        (p_program->m_rom[TM_MAGIC_NUMBER_ADDRESS + 0] != 'T') ||
        (p_program->m_rom[TM_MAGIC_NUMBER_ADDRESS + 1] != 'M') ||
        (p_program->m_rom[TM_MAGIC_NUMBER_ADDRESS + 2] != '0') ||
        (p_program->m_rom[TM_MAGIC_NUMBER_ADDRESS + 3] != '8')
    )
    {
        tm_errorf("tm: program file '%s' is not a valid tm program.\n", p_filename);
        tm_free(p_program->m_rom);
        fclose(l_file);
        return false;
    }

    // Copy program name from ROM.
    if (p_program->m_rom_size >= TM_PROGRAM_NAME_ADDRESS + TM_PROGRAM_NAME_SIZE)
    {
        memcpy(p_program->m_name, p_program->m_rom + TM_PROGRAM_NAME_ADDRESS, TM_PROGRAM_NAME_SIZE);
        p_program->m_name[TM_PROGRAM_NAME_SIZE] = '\0';
    }
    else
    {
        tm_errorf("tm: program file '%s' is too small to contain a valid program name.\n", p_filename);
        tm_free(p_program->m_rom);
        fclose(l_file);
        return false;
    }

    // Copy program author from ROM.
    if (p_program->m_rom_size >= TM_PROGRAM_AUTHOR_ADDRESS + TM_PROGRAM_AUTHOR_SIZE)
    {
        memcpy(p_program->m_author, p_program->m_rom + TM_PROGRAM_AUTHOR_ADDRESS, TM_PROGRAM_AUTHOR_SIZE);
        p_program->m_author[TM_PROGRAM_AUTHOR_SIZE] = '\0';
    }
    else
    {
        tm_errorf("tm: program file '%s' is too small to contain a valid program author.\n", p_filename);
        tm_free(p_program->m_rom);
        fclose(l_file);
        return false;
    }

    fclose(l_file);
    return true;
}

void tm_destroy_program (tm_program_t* p_program)
{
    if (p_program != nullptr)
    {
        tm_free(p_program->m_rom);
        p_program->m_rom = nullptr;
        tm_free(p_program);
        p_program = nullptr;
    }
}

bool tm_read_rom_byte (tm_program_t* p_program, addr_t p_address, byte_t* p_byte)
{
    tm_expect(p_program != nullptr, "tm: program structure is null!\n");
    tm_expect(p_program->m_rom != nullptr, "tm: program rom buffer is null!\n");
    tm_expect(p_byte != nullptr, "tm: byte pointer is null!\n");
    
    if (p_address >= p_program->m_rom_size)
    {
        tm_errorf("tm: rom read address $%08X is out of bounds.\n", p_address);
        return false;
    }

    *p_byte = p_program->m_rom[p_address];
    return true;
}

bool tm_write_rom_byte (tm_program_t* p_program, addr_t p_address, byte_t p_byte)
{
    tm_expect(p_program != nullptr, "tm: program structure is null!\n");
    tm_expect(p_program->m_rom != nullptr, "tm: program rom buffer is null!\n");

    if (p_address >= p_program->m_rom_size)
    {
        tm_errorf("tm: rom write address $%08X is out of bounds.\n", p_address);
        return false;
    }

    p_program->m_rom[p_address] = p_byte;
    return true;
}