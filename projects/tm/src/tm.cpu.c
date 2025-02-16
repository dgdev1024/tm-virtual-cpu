/// @file tm.cpu.c

#include <tm.cpu.h>

/* TM Registers Structure *****************************************************/

typedef struct tm_registers
{
    long_t          m_a;
    long_t          m_b;
    long_t          m_c;
    long_t          m_d;
    long_t          m_pc;
    long_t          m_ia;
    long_t          m_ma;
    long_t          m_md;
    long_t          m_sp;
    long_t          m_rp;
    word_t          m_ci;
    word_t          m_ie;
    word_t          m_if;
    byte_t          m_ec;
} tm_registers_t;

/* TM Flags Union *************************************************************/

typedef union tm_flags
{
    struct
    {
        byte_t      m_zero          : 1;
        byte_t      m_negative      : 1;
        byte_t      m_half_carry    : 1;
        byte_t      m_carry         : 1;
        byte_t      m_overflow      : 1;
        byte_t      m_underflow     : 1;
        byte_t      m_halt          : 1;
        byte_t      m_stop          : 1;
    };

    byte_t          m_state;
} tm_flags_t;

/* TM CPU Structure ***********************************************************/

typedef struct tm_cpu
{
    tm_bus_read     m_read;
    tm_bus_write    m_write;
    tm_cycle        m_cycle;
    tm_registers_t  m_registers;
    tm_flags_t      m_flags;
    byte_t          m_inst;
    byte_t          m_param1;
    byte_t          m_param2;
    bool            m_da;
    bool            m_ime;
    bool            m_enable_ime;
} tm_cpu_t;

/* Static Functions - Error Checking ******************************************/

static bool tm_set_error (tm_cpu_t* p_cpu, byte_t p_ec)
{
    p_cpu->m_registers.m_ec = p_ec;
    p_cpu->m_flags.m_stop = true;
    return (p_ec == TM_ERROR_OK);
}

/* Static Functions - Bounds Checking *****************************************/

static bool tm_check_readable (tm_cpu_t* p_cpu, addr_t p_address, size_t p_size)
{
    if (
        p_address < TM_PROGRAM_START ||
        (
            p_address + p_size > TM_STACK_START &&
            p_address < TM_QRAM_START
        ) ||
        p_address + p_size > TM_IO_START
    )
    {
        return tm_set_error(p_cpu, TM_ERROR_READ_ACCESS_VIOLATION);
    }

    return true;
}

static bool tm_check_writable (tm_cpu_t* p_cpu, addr_t p_address, size_t p_size)
{
    if (
        p_address < TM_RAM_START ||
        (
            p_address + p_size > TM_STACK_START &&
            p_address < TM_QRAM_START
        ) ||
        p_address + p_size > TM_IO_START
    )
    {
        return tm_set_error(p_cpu, TM_ERROR_WRITE_ACCESS_VIOLATION);
    }

    return true;
}

static bool tm_check_executable (tm_cpu_t* p_cpu, addr_t p_address)
{
    if (
        p_address < TM_PROGRAM_START ||
        (
            p_address + 2 > TM_RAM_START &&
            p_address < TM_XRAM_START
        ) ||
        p_address + 2 > TM_STACK_START
    )
    {
        return tm_set_error(p_cpu, TM_ERROR_EXECUTE_ACCESS_VIOLATION);
    }

    return true;
}

/* Static Functions - Stack Operations ****************************************/

static bool tm_pop_data (tm_cpu_t* p_cpu, long_t* p_data)
{
    if (p_cpu->m_registers.m_sp >= 0x10000)
    {
        return tm_set_error(p_cpu, TM_ERROR_DATA_STACK_UNDERFLOW);
    }

    bool l_result = tm_read_long(
        p_cpu,
        p_cpu->m_registers.m_sp + TM_STACK_START,
        p_data
    );
    if (l_result == true) { p_cpu->m_registers.m_sp += 4; }
    return l_result;
}

static bool tm_push_data (tm_cpu_t* p_cpu, long_t p_data)
{
    if (p_cpu->m_registers.m_sp == 0)
    {
        return tm_set_error(p_cpu, TM_ERROR_DATA_STACK_OVERFLOW);
    }

    p_cpu->m_registers.m_sp -= 4;
    return tm_write_long(
        p_cpu,
        p_cpu->m_registers.m_sp + TM_STACK_START,
        p_data
    );
}

static bool tm_pop_address (tm_cpu_t* p_cpu, addr_t* p_address)
{
    if (p_cpu->m_registers.m_rp >= 0x10000)
    {
        return tm_set_error(p_cpu, TM_ERROR_CALL_STACK_UNDERFLOW);
    }

    bool l_result = tm_read_long(
        p_cpu,
        p_cpu->m_registers.m_rp + TM_CALL_STACK_START,
        p_address
    );
    if (l_result == true) { p_cpu->m_registers.m_rp += 4; }
    return l_result;
}

static bool tm_push_address (tm_cpu_t* p_cpu, addr_t p_address)
{
    if (p_cpu->m_registers.m_rp == 0)
    {
        return tm_set_error(p_cpu, TM_ERROR_CALL_STACK_OVERFLOW);
    }

    p_cpu->m_registers.m_rp -= 4;
    return tm_write_long(
        p_cpu,
        p_cpu->m_registers.m_rp + TM_CALL_STACK_START,
        p_address
    );
}

/* Static Functions - Interrupts and Program Counter Functions ****************/

static bool tm_check_condition (tm_cpu_t* p_cpu, enum_t p_condition)
{
    switch (p_condition)
    {
        case TM_CONDITION_N:    return true;
        case TM_CONDITION_CS:   return p_cpu->m_flags.m_carry == true;
        case TM_CONDITION_CC:   return p_cpu->m_flags.m_carry == false;
        case TM_CONDITION_ZS:   return p_cpu->m_flags.m_zero == true;
        case TM_CONDITION_ZC:   return p_cpu->m_flags.m_zero == false;
        case TM_CONDITION_OS:   return p_cpu->m_flags.m_overflow == true;
        case TM_CONDITION_US:   return p_cpu->m_flags.m_underflow == true;
        default:                return false;
    }
}

static void tm_handle_interrupts (tm_cpu_t* p_cpu)
{
    for (uint8_t i = 0; i < 16; ++i)
    {
        if (
            tm_check_bit(p_cpu->m_registers.m_if, i) == true &&
            tm_check_bit(p_cpu->m_registers.m_ie, i) == true
        )
        {
            tm_push_address(p_cpu, p_cpu->m_registers.m_pc);
            p_cpu->m_registers.m_pc = TM_INT_START + (0x100 * i);

            tm_set_bit(p_cpu->m_registers.m_if, i, false);
            p_cpu->m_flags.m_halt = false;
            p_cpu->m_ime = false;

            return;
        }
    }
}

/* Static Functions - Fetching Operands ***************************************/

static bool tm_fetch_imm8 (tm_cpu_t* p_cpu)
{
    return
        tm_read_byte(p_cpu, p_cpu->m_registers.m_pc, &p_cpu->m_registers.m_md) &&
        tm_advance_cpu(p_cpu, 1);
}

static bool tm_fetch_imm16 (tm_cpu_t* p_cpu)
{
    return
        tm_read_word(p_cpu, p_cpu->m_registers.m_pc, &p_cpu->m_registers.m_md) &&
        tm_advance_cpu(p_cpu, 2);
}

static bool tm_fetch_imm32 (tm_cpu_t* p_cpu)
{
    return
        tm_read_long(p_cpu, p_cpu->m_registers.m_pc, &p_cpu->m_registers.m_md) &&
        tm_advance_cpu(p_cpu, 4);
}

static bool tm_fetch_reg (tm_cpu_t* p_cpu)
{
    return tm_read_cpu_register(p_cpu, p_cpu->m_param2, &p_cpu->m_registers.m_md);
}

static bool tm_fetch_addr32 (tm_cpu_t* p_cpu)
{
    bool l_good =
        tm_read_long(p_cpu, p_cpu->m_registers.m_pc, &p_cpu->m_registers.m_ma) &&
        tm_advance_cpu(p_cpu, 4);

    return l_good;
}

static bool tm_fetch_regptr32 (tm_cpu_t* p_cpu)
{
    bool l_good = 
        (p_cpu->m_param2 & 0b11) == 0 &&
        tm_read_cpu_register(p_cpu, p_cpu->m_param2, &p_cpu->m_registers.m_ma) &&
        tm_check_readable(p_cpu, p_cpu->m_registers.m_ma, 4);

    return l_good;
}


static bool tm_fetch_reg_imm (tm_cpu_t* p_cpu)
{
    bool l_good = false;
    switch (p_cpu->m_param1 & 0b11)
    {
        case 0:
        {
            l_good =
                tm_read_long(p_cpu, p_cpu->m_registers.m_pc, &p_cpu->m_registers.m_md) &&
                tm_advance_cpu(p_cpu, 4);
        } break;
        case 1:
        {
            l_good =
                tm_read_word(p_cpu, p_cpu->m_registers.m_pc, &p_cpu->m_registers.m_md) &&
                tm_advance_cpu(p_cpu, 2);
        } break;
        default:
        {
            l_good =
                tm_read_byte(p_cpu, p_cpu->m_registers.m_pc, &p_cpu->m_registers.m_md) &&
                tm_advance_cpu(p_cpu, 1);
        } break;
    }

    return l_good;
}

static bool tm_fetch_reg_addr8 (tm_cpu_t* p_cpu)
{
    bool l_good =
        tm_read_byte(p_cpu, p_cpu->m_registers.m_pc, &p_cpu->m_registers.m_ma) &&
        tm_advance_cpu(p_cpu, 1);

    if (l_good == true)
    {
        p_cpu->m_registers.m_ma += TM_IO_START;

        switch (p_cpu->m_param1 & 0b11)
        {
            case 0:
            {
                l_good =
                    tm_read_long(p_cpu, p_cpu->m_registers.m_ma, &p_cpu->m_registers.m_md) &&
                    tm_cycle_cpu(p_cpu, 4);
            } break;
            case 1:
            {
                l_good =
                    tm_read_word(p_cpu, p_cpu->m_registers.m_ma, &p_cpu->m_registers.m_md) &&
                    tm_cycle_cpu(p_cpu, 2);
            } break;
            default:
            {
                l_good =
                    tm_read_byte(p_cpu, p_cpu->m_registers.m_ma, &p_cpu->m_registers.m_md) &&
                    tm_cycle_cpu(p_cpu, 1);
            } break;
        }
    }

    return l_good;
}

static bool tm_fetch_reg_addr16 (tm_cpu_t* p_cpu)
{
    bool l_good =
        tm_read_word(p_cpu, p_cpu->m_registers.m_pc, &p_cpu->m_registers.m_ma) &&
        tm_advance_cpu(p_cpu, 2);

    if (l_good == true)
    {
        p_cpu->m_registers.m_ma += TM_QRAM_START;

        switch (p_cpu->m_param1 & 0b11)
        {
            case 0:
            {
                l_good =
                    tm_read_long(p_cpu, p_cpu->m_registers.m_ma, &p_cpu->m_registers.m_md) &&
                    tm_cycle_cpu(p_cpu, 4);
            } break;
            case 1:
            {
                l_good =
                    tm_read_word(p_cpu, p_cpu->m_registers.m_ma, &p_cpu->m_registers.m_md) &&
                    tm_cycle_cpu(p_cpu, 2);
            } break;
            default:
            {
                l_good =
                    tm_read_byte(p_cpu, p_cpu->m_registers.m_ma, &p_cpu->m_registers.m_md) &&
                    tm_cycle_cpu(p_cpu, 1);
            } break;
        }
    }

    return l_good;
}

static bool tm_fetch_reg_addr32 (tm_cpu_t* p_cpu)
{
    bool l_good =
        tm_read_long(p_cpu, p_cpu->m_registers.m_pc, &p_cpu->m_registers.m_ma) &&
        tm_advance_cpu(p_cpu, 4);

    if (l_good == true)
    {
        switch (p_cpu->m_param1 & 0b11)
        {
            case 0:
            {
                l_good =
                    tm_check_readable(p_cpu, p_cpu->m_registers.m_ma, 4) &&
                    tm_read_long(p_cpu, p_cpu->m_registers.m_ma, &p_cpu->m_registers.m_md) &&
                    tm_cycle_cpu(p_cpu, 4);
            } break;
            case 1:
            {
                l_good =
                    tm_check_readable(p_cpu, p_cpu->m_registers.m_ma, 2) &&
                    tm_read_word(p_cpu, p_cpu->m_registers.m_ma, &p_cpu->m_registers.m_md) &&
                    tm_cycle_cpu(p_cpu, 2);
            } break;
            default:
            {
                l_good =
                    tm_check_readable(p_cpu, p_cpu->m_registers.m_ma, 1) &&
                    tm_read_byte(p_cpu, p_cpu->m_registers.m_ma, &p_cpu->m_registers.m_md) &&
                    tm_cycle_cpu(p_cpu, 1);
            } break;
        }
    }

    return l_good;
}

static bool tm_fetch_reg_regptr32 (tm_cpu_t* p_cpu)
{
    bool l_good = 
        (p_cpu->m_param2 & 0b11) == 0 &&
        tm_read_cpu_register(p_cpu, p_cpu->m_param2, &p_cpu->m_registers.m_ma) &&
        tm_check_readable(p_cpu, p_cpu->m_registers.m_ma, 4);

    if (l_good == true)
    {
        switch (p_cpu->m_param1 & 0b11)
        {
            case 0:
            {
                l_good =
                    tm_check_readable(p_cpu, p_cpu->m_registers.m_ma, 4) &&
                    tm_read_long(p_cpu, p_cpu->m_registers.m_ma, &p_cpu->m_registers.m_md) &&
                    tm_cycle_cpu(p_cpu, 4);
            } break;
            case 1:
            {
                l_good =
                    tm_check_readable(p_cpu, p_cpu->m_registers.m_ma, 2) &&
                    tm_read_word(p_cpu, p_cpu->m_registers.m_ma, &p_cpu->m_registers.m_md) &&
                    tm_cycle_cpu(p_cpu, 2);
            } break;
            default:
            {
                l_good =
                    tm_check_readable(p_cpu, p_cpu->m_registers.m_ma, 1) &&
                    tm_read_byte(p_cpu, p_cpu->m_registers.m_ma, &p_cpu->m_registers.m_md) &&
                    tm_cycle_cpu(p_cpu, 1);
            } break;
        }
    }

    return l_good;
}

static bool tm_fetch_addr8_reg (tm_cpu_t* p_cpu)
{
    bool l_good =
        tm_read_cpu_register(p_cpu, p_cpu->m_param2, &p_cpu->m_registers.m_md) &&
        tm_read_byte(p_cpu, p_cpu->m_registers.m_pc, &p_cpu->m_registers.m_ma) &&
        tm_advance_cpu(p_cpu, 1);

    if (l_good == true)
    {
        p_cpu->m_registers.m_ma += TM_IO_START;
    }

    p_cpu->m_da = l_good;
    return l_good;
}

static bool tm_fetch_addr16_reg (tm_cpu_t* p_cpu)
{
    bool l_good =
        tm_read_cpu_register(p_cpu, p_cpu->m_param2, &p_cpu->m_registers.m_md) &&
        tm_read_word(p_cpu, p_cpu->m_registers.m_pc, &p_cpu->m_registers.m_ma) &&
        tm_advance_cpu(p_cpu, 2);

    if (l_good == true)
    {
        p_cpu->m_registers.m_ma += TM_QRAM_START;
    }

    p_cpu->m_da = l_good;
    return l_good;
}

static bool tm_fetch_addr32_reg (tm_cpu_t* p_cpu)
{
    bool l_good =
        tm_read_cpu_register(p_cpu, p_cpu->m_param2, &p_cpu->m_registers.m_md) &&
        tm_read_long(p_cpu, p_cpu->m_registers.m_pc, &p_cpu->m_registers.m_ma) &&
        tm_advance_cpu(p_cpu, 4);

    switch (p_cpu->m_param2 & 0b11)
    {
        case 0:     l_good = tm_check_writable(p_cpu, p_cpu->m_registers.m_ma, 4); break;
        case 1:     l_good = tm_check_writable(p_cpu, p_cpu->m_registers.m_ma, 2); break;
        default:    l_good = tm_check_writable(p_cpu, p_cpu->m_registers.m_ma, 1); break;
    }

    p_cpu->m_da = l_good;
    return l_good;
}

static bool tm_fetch_regptr32_reg (tm_cpu_t* p_cpu)
{
    bool l_good =
        (p_cpu->m_param1 & 0b11) == 0 &&
        tm_read_cpu_register(p_cpu, p_cpu->m_param2, &p_cpu->m_registers.m_md) &&
        tm_read_cpu_register(p_cpu, p_cpu->m_param1, &p_cpu->m_registers.m_ma);

    switch (p_cpu->m_param2 & 0b11)
    {
        case 0:     l_good = tm_check_writable(p_cpu, p_cpu->m_registers.m_ma, 4); break;
        case 1:     l_good = tm_check_writable(p_cpu, p_cpu->m_registers.m_ma, 2); break;
        default:    l_good = tm_check_writable(p_cpu, p_cpu->m_registers.m_ma, 1); break;
    }

    p_cpu->m_da = l_good;
    return l_good;
}

/* Static Functions - Instruction Execution ***********************************/

static bool tm_execute_nop (tm_cpu_t* p_cpu)
{
    // The `NOP` instruction does nothing.
    return true;
}

static bool tm_execute_stop (tm_cpu_t* p_cpu)
{
    // The `STOP` instruction sets the TM's stop flag, indicating that the
    // program being run has exited. The CPU will not run at all when this flag
    // is set.
    p_cpu->m_flags.m_stop = true;
    return true;
}

static bool tm_execute_halt (tm_cpu_t* p_cpu)
{
    // The `HALT` instruction sets the TM's halt flag. The CPU will not execute
    // instructions while this flag is set, and the flag will remain set until
    // an interrupt is requested.
    p_cpu->m_flags.m_halt = true;
    return true;
}

static bool tm_execute_sec (tm_cpu_t* p_cpu)
{
    // In the case of the `SEC` instruction, the error code is stored in the
    // low byte of the current instruction register (CIR).
    p_cpu->m_registers.m_ec = tm_check_byte(p_cpu->m_registers.m_ci, 0);
    return true;
}

static bool tm_execute_cec (tm_cpu_t* p_cpu)
{
    // The `CEC` instruction sets the error code register to zero.
    p_cpu->m_registers.m_ec = 0;
    return true;
}

static bool tm_execute_di (tm_cpu_t* p_cpu)
{
    // The `DI` instruction disables the TM's interrupt master flag. When this
    // flag is disabled, interrupts can still be requested, but will not be
    // handled until this flag (and the interrupt's individual enable flag)
    // is re-enabled.
    p_cpu->m_ime = false;
    return true;
}

static bool tm_execute_ei (tm_cpu_t* p_cpu)
{
    // The `EI` instruction enables the TM's interrupt master flag at the end
    // of the current execution step. Any interrupts currently requested will
    // be handled at the end of the next step.
    p_cpu->m_enable_ime = true;
    return true;
}

static bool tm_execute_daa (tm_cpu_t* p_cpu)
{
    // The `DAA` instruction performs a decimal-adjustment on the byte
    // accumulator register `AL`, so its value is representable as a
    // "binary-coded decimal". A binary-coded decimal (BCD) is a hexadecimal
    // value in which all of its nibbles are numeric - that is, a number between
    // 0 and 9.

    // Fetch the value of `AL`.
    long_t l_al = 0;
    tm_read_cpu_register(p_cpu, TM_REGISTER_AL, &l_al);

    // Keep track of the amount by which `AL` needs to be adjusted, and the
    // result of the adjustment.
    long_t l_al_adjust = 0;
    long_t l_al_result = 0;

    // The decimal adjustment is performed on a per-nibble basis. Start with the
    // lower nibble of `AL`. If the nibble's value is a letter (between A and F),
    // or if the half-carry flag is set, then that nibble needs to be adjusted.
    if (p_cpu->m_flags.m_half_carry == true || ((l_al & 0x0F) > 0x09))
    {
        l_al_adjust += 0x06;
    }

    // Next, check to see if the upper nibble needs to be decimal adjusted. It
    // will need to be if that nibble is between A and F, or if the carry flag
    // is set. Also, set the carry flag accordingly.
    if (p_cpu->m_flags.m_carry == true || ((l_al & 0xF0) > 0x90))
    {
        p_cpu->m_flags.m_carry = true;
        l_al_adjust += 0x60;
    }
    else
    {
        p_cpu->m_flags.m_carry = false;
    }

    // Depending on the state of the subtraction flag, the adjust value will be
    // either added to or subtracted from `AL`.
    l_al_result = (p_cpu->m_flags.m_negative == true) ?
        (l_al - l_al_adjust) : (l_al + l_al_adjust);

    // Write the result to `AL`, then set flags.
    tm_write_cpu_register(p_cpu, TM_REGISTER_AL, l_al_result);
    p_cpu->m_flags.m_zero = (tm_check_byte(l_al_result, 0) == 0);
    p_cpu->m_flags.m_half_carry = false;
    p_cpu->m_flags.m_overflow =
        (p_cpu->m_flags.m_carry == true && p_cpu->m_flags.m_negative == false);
    p_cpu->m_flags.m_underflow =
        (p_cpu->m_flags.m_carry == true && p_cpu->m_flags.m_negative == true);

    return true;
}

static bool tm_execute_cpl (tm_cpu_t* p_cpu)
{
    // The `CPL` instruction compliments the bits of the long accumulator
    // register `A` - that is, it clears the set bits and sets the clear bits.

    long_t l_a = 0;
    tm_read_cpu_register(p_cpu, TM_REGISTER_A, &l_a);
    tm_write_cpu_register(p_cpu, TM_REGISTER_A, ~l_a);

    p_cpu->m_flags.m_negative = true;
    p_cpu->m_flags.m_half_carry = true;
    return true;
}

static bool tm_execute_cpw (tm_cpu_t* p_cpu)
{
    // The `CPW` instruction compliments the bits of the word accumulator
    // register `AW`.

    long_t l_aw = 0;
    tm_read_cpu_register(p_cpu, TM_REGISTER_AW, &l_aw);
    tm_write_cpu_register(p_cpu, TM_REGISTER_AW, ~l_aw);

    p_cpu->m_flags.m_negative = true;
    p_cpu->m_flags.m_half_carry = true;
    return true;
}

static bool tm_execute_cpb (tm_cpu_t* p_cpu)
{
    // The `CPB` instruction compliments the bits of the byte accumulator
    // register `AL`.

    long_t l_al = 0;
    tm_read_cpu_register(p_cpu, TM_REGISTER_AL, &l_al);
    tm_write_cpu_register(p_cpu, TM_REGISTER_AL, ~l_al);

    p_cpu->m_flags.m_negative = true;
    p_cpu->m_flags.m_half_carry = true;
    return true;
}

static bool tm_execute_scf (tm_cpu_t* p_cpu)
{
    // The `SCF` instruction sets the carry flag. It also clears the subtraction,
    // half-carry, overflow and underflow flags.
    p_cpu->m_flags.m_negative   = false;
    p_cpu->m_flags.m_half_carry = false;
    p_cpu->m_flags.m_carry      = true;
    p_cpu->m_flags.m_overflow   = false;
    p_cpu->m_flags.m_underflow  = false;
    return true;
}

static bool tm_execute_ccf (tm_cpu_t* p_cpu)
{
    // The `CCF` instruction compliments the carry flag, clearing the flag it it's
    // set and setting the flag if it's clear. It also clears the subtraction,
    // half-carry, overflow and underflow flags.
    p_cpu->m_flags.m_negative   = false;
    p_cpu->m_flags.m_half_carry = false;
    p_cpu->m_flags.m_carry      = !p_cpu->m_flags.m_carry;
    p_cpu->m_flags.m_overflow   = false;
    p_cpu->m_flags.m_underflow  = false;
    return true;
}

static bool tm_execute_ld (tm_cpu_t* p_cpu)
{
    // The `LD` instruction copies the value stored in the memory data register
    // (MDR) into a destination register specified by the instruction's first
    // parameter.

    // The `LDQ` and `LDH` instructions are variants of the `LD` instruction
    // used for quickly loading data from quick RAM (QRAM) and from the IO
    // port registers.

    return tm_write_cpu_register(
        p_cpu, 
        p_cpu->m_param1, 
        p_cpu->m_registers.m_md
    );
}

static bool tm_execute_st (tm_cpu_t* p_cpu)
{
    // The `ST` instruction copies the value stored in the memory data register
    // (MDR) into memory at an address specified by the memory address register
    // (MAR).
    
    // The `STQ` and `STH` instructions are variants of the `ST` instruction
    // used for quickly storing data in quick RAM (QRAM) and in the IO port
    // registers.

    bool l_good = true;
    switch (p_cpu->m_param2 & 0b11)
    {
        case 0:
            l_good =
                tm_write_long(p_cpu, p_cpu->m_registers.m_ma, p_cpu->m_registers.m_md) &&
                tm_cycle_cpu(p_cpu, 4);
            break;
        case 1:
            l_good =
                tm_write_word(p_cpu, p_cpu->m_registers.m_ma, p_cpu->m_registers.m_md) &&
                tm_cycle_cpu(p_cpu, 2);
            break;
        default:
            l_good =
                tm_write_word(p_cpu, p_cpu->m_registers.m_ma, p_cpu->m_registers.m_md) &&
                tm_cycle_cpu(p_cpu, 1);
            break;
    }

    return l_good;
}

static bool tm_execute_mv (tm_cpu_t* p_cpu)
{
    // The `MV` instruction copies the value stored in the memory data register
    // (MDR) into a destination register specified by parameter #1.

    return tm_write_cpu_register(
        p_cpu, 
        p_cpu->m_param1, 
        p_cpu->m_registers.m_md
    );
}

static bool tm_execute_push (tm_cpu_t* p_cpu)
{
    // The `PUSH` instruction pushes the value stored in the memory data register
    // into the data stack.

    return tm_push_data(p_cpu, p_cpu->m_registers.m_md) && tm_cycle_cpu(p_cpu, 5);
}

static bool tm_execute_pop (tm_cpu_t* p_cpu)
{
    // The `POP` instruction copies the value in the memory data register (MDR),
    // which was just popped from the data stack, into the destination register
    // specified by parameter #1.

    return
        tm_pop_data(p_cpu, &p_cpu->m_registers.m_md) &&
        tm_cycle_cpu(p_cpu, 5) &&
        tm_write_cpu_register(p_cpu, p_cpu->m_param1, p_cpu->m_registers.m_md);
}

static bool tm_execute_jmp (tm_cpu_t* p_cpu)
{
    // The `JMP` instruction moves the program counter to a memory location
    // specified by the memory data register, provided the condition specified
    // by parameter #1 is fulfilled.

    if (tm_check_condition(p_cpu, p_cpu->m_param1) == true)
    {
        p_cpu->m_registers.m_pc = p_cpu->m_registers.m_ma;
        return tm_cycle_cpu(p_cpu, 1);
    }

    return true;
}

static bool tm_execute_jpb (tm_cpu_t* p_cpu)
{
    // The `JPB` instruction moves the program counter by a signed offset
    // specified by the memory data register, provided the condition specified
    // by parameter #1 is fulfilled.

    if (tm_check_condition(p_cpu, p_cpu->m_param1) == true)
    {
        p_cpu->m_registers.m_pc += (int16_t) p_cpu->m_registers.m_md;
        return tm_cycle_cpu(p_cpu, 1);
    }

    return true;
}

static bool tm_execute_call (tm_cpu_t* p_cpu)
{
    // The `CALL` instruction calls a subroutine starting at an address specified
    // by the memory address register (MAR), provided the condition specified by
    // parameter #1 is fulfilled, and that the call stack is not full.

    if (tm_check_condition(p_cpu, p_cpu->m_param1) == true)
    {
        if (
            tm_push_address(p_cpu, p_cpu->m_registers.m_pc) == false ||
            tm_cycle_cpu(p_cpu, 5) == false
        )
        {
            return false;
        }

        p_cpu->m_registers.m_pc = p_cpu->m_registers.m_ma;
        return tm_cycle_cpu(p_cpu, 1);
    }

    return true;
}

/* Public Functions ***********************************************************/

tm_cpu_t* tm_create_cpu (tm_bus_read p_read, tm_bus_write p_write, tm_cycle p_cycle)
{
    tm_expect(p_read, "tm: expected a bus read function!\n");
    tm_expect(p_write, "tm: expected a bus write function!\n");
    tm_expect(p_cycle, "tm: expected a bus cycle function!\n");

    tm_cpu_t* l_cpu = tm_calloc(1, tm_cpu_t);
    tm_expect_p(l_cpu, "tm: could not allocate cpu context");

    l_cpu->m_read   = p_read;
    l_cpu->m_write  = p_write;
    l_cpu->m_cycle  = p_cycle;
    tm_init_cpu(l_cpu);

    return l_cpu;
}

void tm_init_cpu (tm_cpu_t* p_cpu)
{
    tm_hard_assert(p_cpu);

    // 1. Reset all CPU registers and the flags register to zero.
    memset(&p_cpu->m_registers, 0x00, sizeof(tm_registers_t));
    memset(&p_cpu->m_flags, 0x00, sizeof(tm_flags_t));

    // 2. Set the program counter, stack pointer and current instruction
    //    registers to their initial values.
    p_cpu->m_registers.m_pc = TM_PROGRAM_START;
    p_cpu->m_registers.m_sp = 0x10000;
    p_cpu->m_registers.m_rp = 0x10000;
    p_cpu->m_registers.m_ci = 0xFFFF;
}

void tm_destroy_cpu (tm_cpu_t* p_cpu)
{
    tm_hard_assert(p_cpu);
    tm_free(p_cpu);
}

/* Public Functions - CPU Registers *******************************************/

bool tm_read_cpu_register (tm_cpu_t* p_cpu, enum_t p_type, long_t* p_value)
{
    tm_assert(p_cpu != nullptr);
    tm_assert(p_value != nullptr);

    switch (p_type)
    {
        case TM_REGISTER_A:     *p_value = p_cpu->m_registers.m_a; break;
        case TM_REGISTER_AW:    *p_value = p_cpu->m_registers.m_a & 0xFFFF; break;
        case TM_REGISTER_AH:    *p_value = (p_cpu->m_registers.m_a >> 8) & 0xFF; break;
        case TM_REGISTER_AL:    *p_value = p_cpu->m_registers.m_a & 0xFF; break;
        case TM_REGISTER_B:     *p_value = p_cpu->m_registers.m_b; break;
        case TM_REGISTER_BW:    *p_value = p_cpu->m_registers.m_b & 0xFFFF; break;
        case TM_REGISTER_BH:    *p_value = (p_cpu->m_registers.m_b >> 8) & 0xFF; break;
        case TM_REGISTER_BL:    *p_value = p_cpu->m_registers.m_b & 0xFF; break;
        case TM_REGISTER_C:     *p_value = p_cpu->m_registers.m_c; break;
        case TM_REGISTER_CW:    *p_value = p_cpu->m_registers.m_c & 0xFFFF; break;
        case TM_REGISTER_CH:    *p_value = (p_cpu->m_registers.m_c >> 8) & 0xFF; break;
        case TM_REGISTER_CL:    *p_value = p_cpu->m_registers.m_c & 0xFF; break;
        case TM_REGISTER_D:     *p_value = p_cpu->m_registers.m_d; break;
        case TM_REGISTER_DW:    *p_value = p_cpu->m_registers.m_d & 0xFFFF; break;
        case TM_REGISTER_DH:    *p_value = (p_cpu->m_registers.m_d >> 8) & 0xFF; break;
        case TM_REGISTER_DL:    *p_value = p_cpu->m_registers.m_d & 0xFF; break;
        default: return false;
    }

    return true;
}

bool tm_write_cpu_register (tm_cpu_t* p_cpu, enum_t p_type, long_t p_value)
{
    tm_assert(p_cpu != nullptr);

    switch (p_type)
    {
        case TM_REGISTER_A:     
            p_cpu->m_registers.m_a = p_value; 
            break;
        case TM_REGISTER_AW:    
            p_cpu->m_registers.m_a = (p_cpu->m_registers.m_a & 0xFFFF0000) | (p_value & 0xFFFF); 
            break;
        case TM_REGISTER_AH:    
            p_cpu->m_registers.m_a = (p_cpu->m_registers.m_a & 0xFFFF00FF) | ((p_value & 0xFF) << 8); 
            break;
        case TM_REGISTER_AL:    
            p_cpu->m_registers.m_a = (p_cpu->m_registers.m_a & 0xFFFFFF00) | (p_value & 0xFF); 
            break;
        case TM_REGISTER_B:     
            p_cpu->m_registers.m_b = p_value; 
            break;
        case TM_REGISTER_BW:    
            p_cpu->m_registers.m_b = (p_cpu->m_registers.m_b & 0xFFFF0000) | (p_value & 0xFFFF); 
            break;
        case TM_REGISTER_BH:    
            p_cpu->m_registers.m_b = (p_cpu->m_registers.m_b & 0xFFFF00FF) | ((p_value & 0xFF) << 8); 
            break;
        case TM_REGISTER_BL:    
            p_cpu->m_registers.m_b = (p_cpu->m_registers.m_b & 0xFFFFFF00) | (p_value & 0xFF); 
            break;
        case TM_REGISTER_C:     
            p_cpu->m_registers.m_c = p_value; 
            break;
        case TM_REGISTER_CW:    
            p_cpu->m_registers.m_c = (p_cpu->m_registers.m_c & 0xFFFF0000) | (p_value & 0xFFFF); 
            break;
        case TM_REGISTER_CH:    
            p_cpu->m_registers.m_c = (p_cpu->m_registers.m_c & 0xFFFF00FF) | ((p_value & 0xFF) << 8); 
            break;
        case TM_REGISTER_CL:    
            p_cpu->m_registers.m_c = (p_cpu->m_registers.m_c & 0xFFFFFF00) | (p_value & 0xFF); 
            break;
        case TM_REGISTER_D:     
            p_cpu->m_registers.m_d = p_value; 
            break;
        case TM_REGISTER_DW:    
            p_cpu->m_registers.m_d = (p_cpu->m_registers.m_d & 0xFFFF0000) | (p_value & 0xFFFF); 
            break;
        case TM_REGISTER_DH:    
            p_cpu->m_registers.m_d = (p_cpu->m_registers.m_d & 0xFFFF00FF) | ((p_value & 0xFF) << 8); 
            break;
        case TM_REGISTER_DL:    
            p_cpu->m_registers.m_d = (p_cpu->m_registers.m_d & 0xFFFFFF00) | (p_value & 0xFF); 
            break;
        default: return false;
    }

    return true;
}

/* Public Functions - Bus Read ************************************************/

bool tm_read_byte (tm_cpu_t* p_cpu, addr_t p_address, long_t* p_value)
{
    tm_assert(p_cpu != nullptr);
    tm_assert(p_value != nullptr);

    if (p_cpu->m_read(p_address, p_value) == false)
    {
        return tm_set_error(p_cpu, TM_ERROR_BUS_READ);
    }

    return true;
}

bool tm_read_word (tm_cpu_t* p_cpu, addr_t p_address, long_t* p_value)
{
    tm_assert(p_cpu != nullptr);
    tm_assert(p_value != nullptr);

    long_t l_byte1 = 0, l_byte0 = 0;
    if (
        p_cpu->m_read(p_address, &l_byte1) == false ||
        p_cpu->m_read(p_address + 1, &l_byte0) == false
    )
    {
        return tm_set_error(p_cpu, TM_ERROR_BUS_READ);
    }

    *p_value = (l_byte1 << 8) | l_byte0;
    return true;
}

bool tm_read_long (tm_cpu_t* p_cpu, addr_t p_address, long_t* p_value)
{
    tm_assert(p_cpu != nullptr);
    tm_assert(p_value != nullptr);

    long_t l_byte3 = 0, l_byte2 = 0, l_byte1 = 0, l_byte0 = 0;
    if (
        p_cpu->m_read(p_address    , &l_byte3) == false ||
        p_cpu->m_read(p_address + 1, &l_byte2) == false ||
        p_cpu->m_read(p_address + 2, &l_byte1) == false ||
        p_cpu->m_read(p_address + 3, &l_byte0) == false
    )
    {
        return tm_set_error(p_cpu, TM_ERROR_BUS_READ);
    }

    *p_value = (l_byte3 << 24) | (l_byte2 << 16) | (l_byte1 << 8) | l_byte0;
    return true;
}

/* Public Functions - Bus Write ***********************************************/

bool tm_write_byte (tm_cpu_t* p_cpu, addr_t p_address, long_t p_value)
{
    tm_assert(p_cpu != nullptr);

    if (p_cpu->m_write(p_address, p_value) == false)
    {
        return tm_set_error(p_cpu, TM_ERROR_BUS_WRITE);
    }

    return true;
}

bool tm_write_word (tm_cpu_t* p_cpu, addr_t p_address, long_t p_value)
{
    tm_assert(p_cpu != nullptr);

    if (
        p_cpu->m_write(p_address, (p_value >> 8) & 0xFF) == false ||
        p_cpu->m_write(p_address + 1, p_value & 0xFF) == false
    )
    {
        return tm_set_error(p_cpu, TM_ERROR_BUS_WRITE);
    }

    return true;
}

bool tm_write_long (tm_cpu_t* p_cpu, addr_t p_address, long_t p_value)
{
    tm_assert(p_cpu != nullptr);

    if (
        p_cpu->m_write(p_address, (p_value >> 24) & 0xFF) == false ||
        p_cpu->m_write(p_address + 1, (p_value >> 16) & 0xFF) == false ||
        p_cpu->m_write(p_address + 2, (p_value >> 8) & 0xFF) == false ||
        p_cpu->m_write(p_address + 3, p_value & 0xFF) == false
    )
    {
        return tm_set_error(p_cpu, TM_ERROR_BUS_WRITE);
    }

    return true;
}

/* Public Functions - Interrupts **********************************************/

void tm_request_interrupt (tm_cpu_t* p_cpu, byte_t p_id)
{
    tm_assert(p_cpu != nullptr);
    tm_set_bit(p_cpu->m_registers.m_if, (p_id & 0xF), true);
}

/* Public Functions - Cycle and Step ******************************************/

bool tm_cycle_cpu (tm_cpu_t* p_cpu, size_t p_cycle_count)
{
    tm_assert(p_cpu != nullptr);
    for (size_t i = 0; i < p_cycle_count; ++i)
    {
        if (p_cpu->m_cycle() == false)
        {
            return tm_set_error(p_cpu, TM_ERROR_HARDWARE);
        }
    }

    return true;
}

bool tm_advance_cpu (tm_cpu_t* p_cpu, size_t p_cycle_count)
{
    tm_assert(p_cpu != nullptr);
    bool l_good = tm_cycle_cpu(p_cpu, p_cycle_count);
    if (l_good == true) { p_cpu->m_registers.m_pc += p_cycle_count; }
    return l_good;
}

bool tm_step_cpu (tm_cpu_t* p_cpu)
{
    tm_assert(p_cpu != nullptr);

    if (p_cpu->m_flags.m_stop == true)
    {
        return false;
    }
    else if (p_cpu->m_flags.m_halt == false)
    {

        // The instruction cycle consists of three repeating stages:
        //
        // 1. Fetch Stage - Read the next instruction from memory.
        // 2. Decode Stage - Determine the instruction to be executed and its parameters.
        // 3. Execute Stage - Fetch necessary operands and execute the instruction.
        // 4. Repeat until program termination.

        // 1a.  The program counter register contains the opcode of the next
        //      instruction to be executed. Copy it into the memory address
        //      register (MAR).
        p_cpu->m_registers.m_ma = p_cpu->m_registers.m_pc;

        // 1b.  Ensure that the memory address retrieved is within executable
        //      bounds. Copy the contents of the MAR into the instruction
        //      address register (IAR) if it is. Throw an execute access 
        //      violation error if it isn't.
        if (tm_check_executable(p_cpu, p_cpu->m_registers.m_ma) == false)
        {
            return false;
        }
        else
        {
            p_cpu->m_registers.m_ia = p_cpu->m_registers.m_ma;
        }

        // 1c.  Read a two-byte opcode from the data bus at the address 
        //      specified in the MAR. Place the word into the memory data 
        //      register (MDR), then advance the program counter by two places, 
        //      as two bytes were read.
        //
        //      For each byte read from or written to the data bus, the CPU is
        //      cycled one time.
        if (tm_read_word(p_cpu, p_cpu->m_registers.m_ma, &p_cpu->m_registers.m_md) == false)
        {
            return false;
        }
        else if (tm_cycle_cpu(p_cpu, 2) == false)
        {
            return false;
        }
        else
        {
            p_cpu->m_registers.m_pc += 2;
        }

        // 1d.  Copy the contents of the MDR into the current instruction
        //      register (CIR).
        p_cpu->m_registers.m_ci = p_cpu->m_registers.m_md;

        // 2.   An instruction's operation code (opcode) contains important
        //      information on the instruction to be executed and its
        //      parameters:
        //          -   The upper byte (0xXX00) identifies the instruction to be
        //              executed.
        //          -   The lower byte (0x00XX) contains information on the
        //              instruction's parameters.
        //          -   In many cases, the upper nibble of this byte (0x00X0)
        //              indicates a destination register, and the lower nibble
        //              (0x000X) indicates a source register.
        //
        //      Decode the encoded instruction from the CIR, and record the
        //      decoded information.
        p_cpu->m_inst   = tm_check_byte(p_cpu->m_registers.m_ci, 1);
        p_cpu->m_param1 = tm_check_nibble(p_cpu->m_registers.m_ci, 1);
        p_cpu->m_param2 = tm_check_nibble(p_cpu->m_registers.m_ci, 0);

        // 3a.  Based on the opcode and parameters, fetch any extra information
        //      needed to execute the instruction, then execute the instruction.
        bool l_good = false;
        switch (p_cpu->m_inst)
        {
            case 0x00:  l_good = tm_execute_nop(p_cpu); break;
            case 0x01:  l_good = tm_execute_stop(p_cpu); break;
            case 0x02:  l_good = tm_execute_halt(p_cpu); break;
            case 0x03:  l_good = tm_execute_sec(p_cpu); break;
            case 0x04:  l_good = tm_execute_cec(p_cpu); break;
            case 0x05:  l_good = tm_execute_di(p_cpu); break;
            case 0x06:  l_good = tm_execute_ei(p_cpu); break;
            case 0x07:  l_good = tm_execute_daa(p_cpu); break;
            case 0x08:  l_good = tm_execute_cpl(p_cpu); break;
            case 0x09:  l_good = tm_execute_cpw(p_cpu); break;
            case 0x0A:  l_good = tm_execute_cpb(p_cpu); break;
            case 0x0B:  l_good = tm_execute_scf(p_cpu); break;
            case 0x0C:  l_good = tm_execute_ccf(p_cpu); break;

            case 0x10:  l_good = tm_fetch_reg_imm(p_cpu)        && tm_execute_ld(p_cpu);    break;
            case 0x11:  l_good = tm_fetch_reg_addr32(p_cpu)     && tm_execute_ld(p_cpu);    break;
            case 0x12:  l_good = tm_fetch_reg_regptr32(p_cpu)   && tm_execute_ld(p_cpu);    break;
            case 0x13:  l_good = tm_fetch_reg_addr16(p_cpu)     && tm_execute_ld(p_cpu);    break;
            case 0x15:  l_good = tm_fetch_reg_addr8(p_cpu)      && tm_execute_ld(p_cpu);    break;
            case 0x17:  l_good = tm_fetch_addr32_reg(p_cpu)     && tm_execute_st(p_cpu);    break;
            case 0x18:  l_good = tm_fetch_regptr32_reg(p_cpu)   && tm_execute_st(p_cpu);    break;
            case 0x19:  l_good = tm_fetch_addr16_reg(p_cpu)     && tm_execute_st(p_cpu);    break;
            case 0x1B:  l_good = tm_fetch_addr8_reg(p_cpu)      && tm_execute_st(p_cpu);    break;
            case 0x1D:  l_good = tm_fetch_reg(p_cpu)            && tm_execute_mv(p_cpu);    break;
            case 0x1E:  l_good = tm_fetch_reg(p_cpu)            && tm_execute_push(p_cpu);  break;
            case 0x1F:  l_good = tm_execute_pop(p_cpu);                                     break;

            case 0x20:  l_good = tm_fetch_addr32(p_cpu)         && tm_execute_jmp(p_cpu);   break;
            case 0x21:  l_good = tm_fetch_regptr32(p_cpu)       && tm_execute_jmp(p_cpu);   break;
            case 0x22:  l_good = tm_fetch_imm16(p_cpu)          && tm_execute_jpb(p_cpu);   break;
            case 0x23:  l_good = tm_fetch_addr32(p_cpu)         && tm_execute_call(p_cpu);  break;

            default:
                return tm_set_error(p_cpu, TM_ERROR_INVALID_OPCODE);
        }

        // 3b.  Ensure that the data fetching and instruction execution were
        //      successful.
        if (l_good == false)
        {
            return false;
        }

    }
    else
    {
        tm_cycle_cpu(p_cpu, 1);
        if (p_cpu->m_registers.m_if != 0)
        {
            p_cpu->m_flags.m_halt = false;
        }
    }

    if (p_cpu->m_ime == true)
    {
        tm_handle_interrupts(p_cpu);
        p_cpu->m_enable_ime = false;
    }

    if (p_cpu->m_enable_ime == true)
    {
        p_cpu->m_ime = true;
    }

    return true;
}
