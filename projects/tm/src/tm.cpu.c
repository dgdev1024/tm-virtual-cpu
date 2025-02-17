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
    long_t          m_ea;
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
    char            m_error_string[TM_ERROR_STRLEN];
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
    // The `tm_set_error` function sets the error code register to the value
    // specified by the parameter `p_ec`, then sets the stop flag to true.

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
        p_cpu->m_registers.m_ea = p_address;
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
        p_cpu->m_registers.m_ea = p_address;
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
        p_cpu->m_registers.m_ea = p_address;
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
        tm_check_readable(p_cpu, p_cpu->m_registers.m_pc, 1) &&
        tm_read_byte(p_cpu, p_cpu->m_registers.m_pc, &p_cpu->m_registers.m_md) &&
        tm_advance_cpu(p_cpu, 1);
}

static bool tm_fetch_imm16 (tm_cpu_t* p_cpu)
{
    return
        tm_check_readable(p_cpu, p_cpu->m_registers.m_pc, 2) &&
        tm_read_word(p_cpu, p_cpu->m_registers.m_pc, &p_cpu->m_registers.m_md) &&
        tm_advance_cpu(p_cpu, 2);
}

static bool tm_fetch_imm32 (tm_cpu_t* p_cpu)
{
    return
        tm_check_readable(p_cpu, p_cpu->m_registers.m_pc, 4) &&
        tm_read_long(p_cpu, p_cpu->m_registers.m_pc, &p_cpu->m_registers.m_md) &&
        tm_advance_cpu(p_cpu, 4);
}

static bool tm_fetch_reg (tm_cpu_t* p_cpu, bool p_second)
{
    return tm_read_cpu_register(
        p_cpu, 
        (p_second == true) ?
            p_cpu->m_param2 :
            p_cpu->m_param1, 
        &p_cpu->m_registers.m_md
    );
}

static bool tm_fetch_addr32 (tm_cpu_t* p_cpu, bool p_dest)
{
    bool l_good =
        tm_read_long(p_cpu, p_cpu->m_registers.m_pc, &p_cpu->m_registers.m_ma) &&
        tm_advance_cpu(p_cpu, 4);

    p_cpu->m_da = p_dest;
    return l_good;
}

static bool tm_fetch_regptr32 (tm_cpu_t* p_cpu, bool p_dest)
{
    bool l_good = 
        (p_cpu->m_param2 & 0b11) == 0 &&
        tm_read_cpu_register(p_cpu, p_cpu->m_param2, &p_cpu->m_registers.m_ma) &&
        tm_check_readable(p_cpu, p_cpu->m_registers.m_ma, 4);

    p_cpu->m_da = p_dest;
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

static bool tm_execute_rst (tm_cpu_t* p_cpu)
{
    // The `RST` instruction calls one of 16 "restart vector" subroutines,
    // located starting at address `$00001000`. The handle of the subroutine to
    // call is encoded in the instruction's first parameter.

    if (
        tm_push_address(p_cpu, p_cpu->m_registers.m_pc) == false ||
        tm_cycle_cpu(p_cpu, 5) == false
    )
    {
        return false;
    }

    p_cpu->m_registers.m_pc = TM_RST_START + (0x100 * p_cpu->m_param1);
    return tm_cycle_cpu(p_cpu, 1);
}

static bool tm_execute_ret (tm_cpu_t* p_cpu)
{
    // The `RET` instruction returns from the current subroutine, by moving the
    // program counter to an address popped from the call stack, provided the
    // condition specified in parameter #1 is fulfilled.

    if (tm_check_condition(p_cpu, p_cpu->m_param1) == true)
    {
        return
            tm_pop_address(p_cpu, &p_cpu->m_registers.m_pc) &&
            tm_cycle_cpu(p_cpu, 6);
    }

    return true;
}

static bool tm_execute_reti (tm_cpu_t* p_cpu)
{
    // The `RETI` instruction enables the CPU's interrupt master flag, then
    // returns from the current subroutine.

    p_cpu->m_ime = true;
    return tm_execute_ret(p_cpu);
}

static bool tm_execute_jps (tm_cpu_t* p_cpu)
{
    // The `JPS` instruction moves the program counter to the start of the
    // program's address space, `$00003000`.

    p_cpu->m_registers.m_pc = TM_PROGRAM_START;
    return tm_cycle_cpu(p_cpu, 1);
}

static bool tm_execute_inc (tm_cpu_t* p_cpu)
{
    // The `INC` instruction increments the value of the memory data register
    // (MDR) by 1.
    //
    // -    If the destination address flag (DA) is set, then the resultant
    //      value is stored in the memory location specified by the memory
    //      address register (MAR).
    // -    Otherwise, the resultant value is stored in a destination register
    //      specified by parameter #1.

    // Add 1 to the MDR's value and store the result.
    uint64_t l_result = p_cpu->m_registers.m_md + 1;

    // An increment does not involve subtraction, so clear the negative flag.  
    p_cpu->m_flags.m_negative = false;

    if (p_cpu->m_da == true)
    {
        p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
        p_cpu->m_flags.m_half_carry = (l_result & 0xF) == 0;

        return
            tm_write_byte(p_cpu, p_cpu->m_registers.m_ma, l_result & 0xFF) &&
            tm_cycle_cpu(p_cpu, 1);
    }

    switch (p_cpu->m_param1 & 0b11)
    {
        case 0:
            p_cpu->m_flags.m_zero = (l_result & 0xFFFFFFFF) == 0;
            break;
        case 1:
            p_cpu->m_flags.m_zero = (l_result & 0xFFFF) == 0;
            break;
        default:
            p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
            p_cpu->m_flags.m_half_carry = (l_result & 0xF) == 0;
            break;
    }

    return tm_write_cpu_register(p_cpu, p_cpu->m_param1, l_result & 0xFFFFFFFF);
}

static bool tm_execute_dec (tm_cpu_t* p_cpu)
{
    // The `DEC` instruction decrements the value of the memory data register
    // (MDR) by 1.
    //
    // -    If the destination address flag (DA) is set, then the resultant
    //      value is stored in the memory location specified by the memory
    //      address register (MAR).
    // -    Otherwise, the resultant value is stored in a destination register
    //      specified by parameter #1.

    // Subtract 1 from the MDR's value and store the result.
    uint64_t l_result = p_cpu->m_registers.m_md - 1;

    // A decrement involves subtraction, so set the negative flag.  
    p_cpu->m_flags.m_negative = true;

    if (p_cpu->m_da == true)
    {
        p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
        p_cpu->m_flags.m_half_carry = (l_result & 0xF) == 0xF;

        return
            tm_write_byte(p_cpu, p_cpu->m_registers.m_ma, l_result & 0xFF) &&
            tm_cycle_cpu(p_cpu, 1);
    }

    switch (p_cpu->m_param1 & 0b11)
    {
        case 0:
            p_cpu->m_flags.m_zero = (l_result & 0xFFFFFFFF) == 0;
            break;
        case 1:
            p_cpu->m_flags.m_zero = (l_result & 0xFFFF) == 0;
            break;
        default:
            p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
            p_cpu->m_flags.m_half_carry = (l_result & 0xF) == 0;
            break;
    }

    return tm_write_cpu_register(p_cpu, p_cpu->m_param1, l_result & 0xFFFFFFFF);
}

static bool tm_execute_add (tm_cpu_t* p_cpu, bool p_with_carry)
{
    // The `ADD` and `ADC` instructions add the value stored in the memory data
    // register (MDR) to the value of a destination accumulator register
    // specified by parameter #1.
    //
    // In the case of the `ADC` instruction, the state of the carry flag is
    // also added.

    // This instruction requires the destination register to be one of the
    // accumulator registers (`A`, `AW`, `AH`, or `AL`). Throw an error if the
    // destination register is not an accumulator.
    if (tm_check_nibble(p_cpu->m_param1, 1) != 0)
    {
        return tm_set_error(p_cpu, TM_ERROR_INVALID_ARGUMENT);
    }

    // Fetch the value of the target accumulator register.
    long_t l_acc_value = 0;
    tm_read_cpu_register(p_cpu, p_cpu->m_param1, &l_acc_value);

    // Add the value of the MDR to the accumulator's value. Add the carry flag
    // if desired. Store the result.
    uint64_t l_result = l_acc_value + p_cpu->m_registers.m_md;
    if (p_with_carry == true) { l_result += p_cpu->m_flags.m_carry; }

    // A second addition operation is needed to calculate a "half-result". This
    // will be used to properly set the half-carry flag.
    uint32_t l_half_result = 0;

    // This is an addition, so clear the negative and underflow flags.
    p_cpu->m_flags.m_negative = false;
    p_cpu->m_flags.m_underflow = false;

    // Write the result back into the accumulator register.
    tm_write_cpu_register(p_cpu, p_cpu->m_param1, l_result & 0xFFFFFFFF);

    // Set flags as appropriate.
    switch (p_cpu->m_param1 & 0b11)
    {
        case 0:
            l_half_result =
                (l_acc_value & 0xFFFFFFF) +
                (p_cpu->m_registers.m_md & 0xFFFFFFF);
            p_cpu->m_flags.m_zero = (l_result & 0xFFFFFFFF) == 0;
            p_cpu->m_flags.m_half_carry = (l_half_result > 0xFFFFFFF);
            p_cpu->m_flags.m_carry = (l_result > 0xFFFFFFFF);
            break;
        case 1:
            l_half_result =
                (l_acc_value & 0xFFF) +
                (p_cpu->m_registers.m_md & 0xFFF);
            p_cpu->m_flags.m_zero = (l_result & 0xFFFF) == 0;
            p_cpu->m_flags.m_half_carry = (l_half_result > 0xFFF);
            p_cpu->m_flags.m_carry = (l_result > 0xFFFF);
            break;
        default:
            l_half_result =
                (l_acc_value & 0xF) +
                (p_cpu->m_registers.m_md & 0xF);
            p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
            p_cpu->m_flags.m_half_carry = (l_half_result > 0xF);
            p_cpu->m_flags.m_carry = (l_result > 0xFF);
            break;
    }

    // Any addition operations which set the carry flag will also set the
    // overflow flag.
    p_cpu->m_flags.m_overflow = p_cpu->m_flags.m_carry;

    return true;
}

static bool tm_execute_sub (tm_cpu_t* p_cpu, bool p_with_carry)
{
    // The `SUB` and `SBC` instructions subtract the value stored in the memory 
    // data register (MDR) from the value of a destination accumulator register
    // specified by parameter #1.
    //
    // In the case of the `SBC` instruction, the state of the carry flag is
    // also subtracted.

    // This instruction requires the destination register to be one of the
    // accumulator registers (`A`, `AW`, `AH`, or `AL`). Throw an error if the
    // destination register is not an accumulator.
    if (tm_check_nibble(p_cpu->m_param1, 1) != 0)
    {
        return tm_set_error(p_cpu, TM_ERROR_INVALID_ARGUMENT);
    }

    // Fetch the value of the target accumulator register.
    long_t l_acc_value = 0;
    tm_read_cpu_register(p_cpu, p_cpu->m_param1, &l_acc_value);

    // Subtract the value of the MDR to the accumulator's value. Subtract the 
    // carry flag if desired. Store the result.
    int64_t l_result = l_acc_value - p_cpu->m_registers.m_md;
    if (p_with_carry == true) { l_result -= p_cpu->m_flags.m_carry; }

    // A second subtraction operation is needed to calculate a "half-result". 
    // This will be used to properly set the half-carry flag.
    int32_t l_half_result = 0;

    // This is a subtraction, so set the negative flag and clear the overflow
    // flag.
    p_cpu->m_flags.m_negative = true;
    p_cpu->m_flags.m_overflow = false;

    // Write the result back into the accumulator register.
    tm_write_cpu_register(p_cpu, p_cpu->m_param1, 
        (uint64_t) l_result & 0xFFFFFFFF);

    // Set flags as appropriate.
    switch (p_cpu->m_param1 & 0b11)
    {
        case 0:
            l_half_result =
                (l_acc_value & 0xFFFFFFF) -
                (p_cpu->m_registers.m_md & 0xFFFFFFF);
            p_cpu->m_flags.m_zero = (l_result & 0xFFFFFFFF) == 0;
            p_cpu->m_flags.m_half_carry = (l_half_result < 0);
            p_cpu->m_flags.m_carry = (l_result < 0);
            break;
        case 1:
            l_half_result =
                (l_acc_value & 0xFFF) -
                (p_cpu->m_registers.m_md & 0xFFF);
            p_cpu->m_flags.m_zero = (l_result & 0xFFFF) == 0;
            p_cpu->m_flags.m_half_carry = (l_half_result < 0);
            p_cpu->m_flags.m_carry = (l_result < 0);
            break;
        default:
            l_half_result =
                (l_acc_value & 0xF) -
                (p_cpu->m_registers.m_md & 0xF);
            p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
            p_cpu->m_flags.m_half_carry = (l_half_result < 0);
            p_cpu->m_flags.m_carry = (l_result < 0);
            break;
    }

    // Any subtraction operations which set the carry flag will also set the
    // underflow flag.
    p_cpu->m_flags.m_underflow = p_cpu->m_flags.m_carry;

    return true;
}

static bool tm_execute_and (tm_cpu_t* p_cpu)
{
    // The `AND` instruction performs a bitwise AND between the destination
    // accumulator register specified by parameter #1 and the value of the
    // memory data register (MDR). The result is then stored in the accumulator
    // register.

    // This instruction requires the destination register to be one of the
    // accumulator registers (`A`, `AW`, `AH`, or `AL`). Throw an error if the
    // destination register is not an accumulator.
    if (tm_check_nibble(p_cpu->m_param1, 1) != 0)
    {
        return tm_set_error(p_cpu, TM_ERROR_INVALID_ARGUMENT);
    }

    // Fetch the value of the target accumulator register.
    long_t l_acc_value = 0;
    tm_read_cpu_register(p_cpu, p_cpu->m_param1, &l_acc_value);

    // Bitwise AND the accumulator and memory data registers' values. Store
    // the result and write it back to the accumulator.
    long_t l_result = (l_acc_value & p_cpu->m_registers.m_md);
    tm_write_cpu_register(p_cpu, p_cpu->m_param1, l_result);

    // Set the zero flag based on the result.
    switch (p_cpu->m_param1 & 0b11)
    {
        case 0:
            p_cpu->m_flags.m_zero = (l_result == 0);
            break;
        case 1:
            p_cpu->m_flags.m_zero = ((l_result & 0xFFFF) == 0);
            break;
        default:
            p_cpu->m_flags.m_zero = ((l_result & 0xFF) == 0);
            break;
    }

    // Set the half-carry flag and clear the negative, carry, overflow and
    // underflow flags.
    p_cpu->m_flags.m_negative = false;
    p_cpu->m_flags.m_half_carry = true;
    p_cpu->m_flags.m_carry = false;
    p_cpu->m_flags.m_overflow = false;
    p_cpu->m_flags.m_underflow = false;

    return true;
}

static bool tm_execute_or (tm_cpu_t* p_cpu)
{
    // The `OR` instruction performs a bitwise OR between the destination
    // accumulator register specified by parameter #1 and the value of the
    // memory data register (MDR). The result is then stored in the accumulator
    // register.

    // This instruction requires the destination register to be one of the
    // accumulator registers (`A`, `AW`, `AH`, or `AL`). Throw an error if the
    // destination register is not an accumulator.
    if (tm_check_nibble(p_cpu->m_param1, 1) != 0)
    {
        return tm_set_error(p_cpu, TM_ERROR_INVALID_ARGUMENT);
    }

    // Fetch the value of the target accumulator register.
    long_t l_acc_value = 0;
    tm_read_cpu_register(p_cpu, p_cpu->m_param1, &l_acc_value);

    // Bitwise OR the accumulator and memory data registers' values. Store
    // the result and write it back to the accumulator.
    long_t l_result = (l_acc_value | p_cpu->m_registers.m_md);
    tm_write_cpu_register(p_cpu, p_cpu->m_param1, l_result);

    // Set the zero flag based on the result.
    switch (p_cpu->m_param1 & 0b11)
    {
        case 0:
            p_cpu->m_flags.m_zero = (l_result == 0);
            break;
        case 1:
            p_cpu->m_flags.m_zero = ((l_result & 0xFFFF) == 0);
            break;
        default:
            p_cpu->m_flags.m_zero = ((l_result & 0xFF) == 0);
            break;
    }

    // Clear the negative, half-carry, carry, overflow and underflow flags.
    p_cpu->m_flags.m_negative = false;
    p_cpu->m_flags.m_half_carry = false;
    p_cpu->m_flags.m_carry = false;
    p_cpu->m_flags.m_overflow = false;
    p_cpu->m_flags.m_underflow = false;

    return true;
}

static bool tm_execute_xor (tm_cpu_t* p_cpu)
{
    // The `XOR` instruction performs a bitwise XOR between the destination
    // accumulator register specified by parameter #1 and the value of the
    // memory data register (MDR). The result is then stored in the accumulator
    // register.

    // This instruction requires the destination register to be one of the
    // accumulator registers (`A`, `AW`, `AH`, or `AL`). Throw an error if the
    // destination register is not an accumulator.
    if (tm_check_nibble(p_cpu->m_param1, 1) != 0)
    {
        return tm_set_error(p_cpu, TM_ERROR_INVALID_ARGUMENT);
    }

    // Fetch the value of the target accumulator register.
    long_t l_acc_value = 0;
    tm_read_cpu_register(p_cpu, p_cpu->m_param1, &l_acc_value);

    // Bitwise XOR the accumulator and memory data registers' values. Store
    // the result and write it back to the accumulator.
    long_t l_result = (l_acc_value ^ p_cpu->m_registers.m_md);
    tm_write_cpu_register(p_cpu, p_cpu->m_param1, l_result);

    // Set the zero flag based on the result.
    switch (p_cpu->m_param1 & 0b11)
    {
        case 0:
            p_cpu->m_flags.m_zero = (l_result == 0);
            break;
        case 1:
            p_cpu->m_flags.m_zero = ((l_result & 0xFFFF) == 0);
            break;
        default:
            p_cpu->m_flags.m_zero = ((l_result & 0xFF) == 0);
            break;
    }

    // Clear the negative, half-carry, carry, overflow and underflow flags.
    p_cpu->m_flags.m_negative = false;
    p_cpu->m_flags.m_half_carry = false;
    p_cpu->m_flags.m_carry = false;
    p_cpu->m_flags.m_overflow = false;
    p_cpu->m_flags.m_underflow = false;

    return true;
}

static bool tm_execute_cmp (tm_cpu_t* p_cpu)
{
    // The `CMP` instruction compares the values of the accumulator register
    // specified by parameter #1 and the memory data register (MDR). This is the
    // same as the `SUB` instruction, only the result is not written back to the
    // accumulator.

    // This instruction requires the destination register to be one of the
    // accumulator registers (`A`, `AW`, `AH`, or `AL`). Throw an error if the
    // destination register is not an accumulator.
    if (tm_check_nibble(p_cpu->m_param1, 1) != 0)
    {
        return tm_set_error(p_cpu, TM_ERROR_INVALID_ARGUMENT);
    }

    // Fetch the value of the target accumulator register.
    long_t l_acc_value = 0;
    tm_read_cpu_register(p_cpu, p_cpu->m_param1, &l_acc_value);

    // Subtract the value of the MDR to the accumulator's value.
    int64_t l_result = l_acc_value - p_cpu->m_registers.m_md;

    // A second subtraction operation is needed to calculate a "half-result". 
    // This will be used to properly set the half-carry flag.
    int32_t l_half_result = 0;

    // The comparison operation involves a subtraction, so set the negative flag 
    // and clear the overflow flag.
    p_cpu->m_flags.m_negative = true;
    p_cpu->m_flags.m_overflow = false;

    // Set flags as appropriate.
    switch (p_cpu->m_param1 & 0b11)
    {
        case 0:
            l_half_result =
                (l_acc_value & 0xFFFFFFF) -
                (p_cpu->m_registers.m_md & 0xFFFFFFF);
            p_cpu->m_flags.m_zero = (l_result & 0xFFFFFFFF) == 0;
            p_cpu->m_flags.m_half_carry = (l_half_result < 0);
            p_cpu->m_flags.m_carry = (l_result < 0);
            break;
        case 1:
            l_half_result =
                (l_acc_value & 0xFFF) -
                (p_cpu->m_registers.m_md & 0xFFF);
            p_cpu->m_flags.m_zero = (l_result & 0xFFFF) == 0;
            p_cpu->m_flags.m_half_carry = (l_half_result < 0);
            p_cpu->m_flags.m_carry = (l_result < 0);
            break;
        default:
            l_half_result =
                (l_acc_value & 0xF) -
                (p_cpu->m_registers.m_md & 0xF);
            p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
            p_cpu->m_flags.m_half_carry = (l_half_result < 0);
            p_cpu->m_flags.m_carry = (l_result < 0);
            break;
    }

    // Any subtraction operations which set the carry flag will also set the
    // underflow flag.
    p_cpu->m_flags.m_underflow = p_cpu->m_flags.m_carry;

    return true;
}

static bool tm_execute_sla (tm_cpu_t* p_cpu)
{
    // The `SLA` instruction shifts the value stored in the memory data register
    // (MDR) left by one bit. The uppermost bit of the old value is stored in
    // the carry flag, and the lowermost bit is cleared.
    // -    If the destination address flag (DA) is set, then the result value
    //      is written to a memory location pointed to by the memory address
    //      register (MAR).
    // -    Otherwise, the result value is stored in the destination register
    //      specified by parameter #1.

    // Shift the MDR's value left one bit. Store the result, with the lowermost
    // bit cleared.
    uint64_t l_result = (p_cpu->m_registers.m_md << 1);
    tm_set_bit(l_result, 0, false);

    // Clear the negative, half-carry, overflow and underflow flags.
    p_cpu->m_flags.m_negative = false;
    p_cpu->m_flags.m_half_carry = false;
    p_cpu->m_flags.m_overflow = false;
    p_cpu->m_flags.m_underflow = false;

    if (p_cpu->m_da == true)
    {
        p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
        p_cpu->m_flags.m_carry = tm_check_bit(l_result, 8);
        return
            tm_write_byte(p_cpu, p_cpu->m_registers.m_ma, l_result & 0xFF) &&
            tm_cycle_cpu(p_cpu, 1);
    }

    switch (p_cpu->m_param1 & 0b11)
    {
        case 0:
            p_cpu->m_flags.m_zero = (l_result & 0xFFFFFFFF) == 0;
            p_cpu->m_flags.m_carry = tm_check_bit(l_result, 32);
            break;
        case 1:
            p_cpu->m_flags.m_zero = (l_result & 0xFFFF) == 0;
            p_cpu->m_flags.m_carry = tm_check_bit(l_result, 16);
            break;
        default:
            p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
            p_cpu->m_flags.m_carry = tm_check_bit(l_result, 8);
            break;
    }

    return tm_write_cpu_register(p_cpu, p_cpu->m_param1, l_result & 0xFFFFFFFF);
}

static bool tm_execute_sra (tm_cpu_t* p_cpu)
{
    // The `SRA` instruction shifts the value stored in the memory data register
    // (MDR) right by one bit. The lowermost bit of the old value is stored in
    // the carry flag, and the uppermost bit is left as is.
    // -    If the destination address flag (DA) is set, then the result value
    //      is written to a memory location pointed to by the memory address
    //      register (MAR).
    // -    Otherwise, the result value is stored in the destination register
    //      specified by parameter #1.
    
    // Keep track of the old value of the MDR, then shift the MDR's value right
    // by one bit and store the result.
    long_t l_old_mdr = p_cpu->m_registers.m_md;
    long_t l_result = (p_cpu->m_registers.m_md >> 1);

    // Set the carry flag to bit 0 of the old value.
    p_cpu->m_flags.m_carry = tm_check_bit(l_old_mdr, 0);

    // Clear the negative, half-carry, overflow and underflow flags.
    p_cpu->m_flags.m_negative = false;
    p_cpu->m_flags.m_half_carry = false;
    p_cpu->m_flags.m_overflow = false;
    p_cpu->m_flags.m_underflow = false;

    // If the destination address flag (DA) is set, write the result to memory
    // at the address specified by the memory address register (MAR) and set the
    // appropriate flags.
    if (p_cpu->m_da == true)
    {
        tm_set_bit(l_result, 7, tm_check_bit(l_old_mdr, 7));
        p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
        return
            tm_write_byte(p_cpu, p_cpu->m_registers.m_ma, l_result & 0xFF) &&
            tm_cycle_cpu(p_cpu, 1);
    }

    // Based on the size of the destination register, set the proper bit of the
    // newly-shifted value to the bit's old value. Also, set the zero flag
    // according to the result.
    switch (p_cpu->m_param1 & 0b11)
    {
        case 0:
            tm_set_bit(l_result, 31, tm_check_bit(l_old_mdr, 31));
            p_cpu->m_flags.m_zero = (l_result & 0xFFFFFFFF) == 0;
            break;
        case 1:
            tm_set_bit(l_result, 15, tm_check_bit(l_old_mdr, 15));
            p_cpu->m_flags.m_zero = (l_result & 0xFFFF) == 0;
            break;
        default:
            tm_set_bit(l_result, 7, tm_check_bit(l_old_mdr, 7));
            p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
            break;
    }
    
    // Write the result back to the destination register.
    return tm_write_cpu_register(p_cpu, p_cpu->m_param1, l_result);
}

static bool tm_execute_srl (tm_cpu_t* p_cpu)
{
    // The `SRL` instruction shifts the value stored in the memory data register
    // (MDR) right by one bit. The lowermost bit of the old value is stored in
    // the carry flag, and the uppermost bit is cleared.
    // -    If the destination address flag (DA) is set, then the result value
    //      is written to a memory location pointed to by the memory address
    //      register (MAR).
    // -    Otherwise, the result value is stored in the destination register
    //      specified by parameter #1.

    // Keep track of the old value of the MDR, then shift the MDR's value right
    // by one bit and store the result.
    long_t l_old_mdr = p_cpu->m_registers.m_md;
    long_t l_result = (p_cpu->m_registers.m_md >> 1);

    // Set the carry flag to bit 0 of the old value.
    p_cpu->m_flags.m_carry = tm_check_bit(l_old_mdr, 0);

    // Clear the negative, half-carry, overflow and underflow flags.
    p_cpu->m_flags.m_negative = false;
    p_cpu->m_flags.m_half_carry = false;
    p_cpu->m_flags.m_overflow = false;
    p_cpu->m_flags.m_underflow = false;

    // If the destination address flag (DA) is set, write the result to memory
    // at the address specified by the memory address register (MAR) and set the
    // appropriate flags.
    if (p_cpu->m_da == true)
    {
        tm_set_bit(l_result, 7, 0);
        p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
        return
            tm_write_byte(p_cpu, p_cpu->m_registers.m_ma, l_result & 0xFF) &&
            tm_cycle_cpu(p_cpu, 1);
    }

    // Based on the size of the destination register, set the proper bit of the
    // newly-shifted value to the bit's old value. Also, set the zero flag
    // according to the result.
    switch (p_cpu->m_param1 & 0b11)
    {
        case 0:
            tm_set_bit(l_result, 31, 0);
            p_cpu->m_flags.m_zero = (l_result & 0xFFFFFFFF) == 0;
            break;
        case 1:
            tm_set_bit(l_result, 15, 0);
            p_cpu->m_flags.m_zero = (l_result & 0xFFFF) == 0;
            break;
        default:
            tm_set_bit(l_result, 7, 0);
            p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
            break;
    }
    
    // Write the result back to the destination register.
    return tm_write_cpu_register(p_cpu, p_cpu->m_param1, l_result);
}

static bool tm_execute_rl (tm_cpu_t* p_cpu)
{
    // The `RL` instruction shifts the value stored in the memory data register
    // (MDR) left by one bit. The uppermost bit of the old value is stored in
    // the carry flag, and the lowermost bit is set to the old value of the
    // carry flag.
    // -    If the destination address flag (DA) is set, then the result value
    //      is written to a memory location pointed to by the memory address
    //      register (MAR).
    // -    Otherwise, the result value is stored in the destination register
    //      specified by parameter #1.

    // Shift the MDR's value left one bit. Store the result, with the lowermost
    // bit set to the current state of the carry flag.
    uint64_t l_result = (p_cpu->m_registers.m_md << 1);
    tm_set_bit(l_result, 0, p_cpu->m_flags.m_carry);

    // Clear the negative, half-carry, overflow and underflow flags.
    p_cpu->m_flags.m_negative = false;
    p_cpu->m_flags.m_half_carry = false;
    p_cpu->m_flags.m_overflow = false;
    p_cpu->m_flags.m_underflow = false;

    if (p_cpu->m_da == true)
    {
        p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
        p_cpu->m_flags.m_carry = tm_check_bit(l_result, 8);
        return
            tm_write_byte(p_cpu, p_cpu->m_registers.m_ma, l_result & 0xFF) &&
            tm_cycle_cpu(p_cpu, 1);
    }

    switch (p_cpu->m_param1 & 0b11)
    {
        case 0:
            p_cpu->m_flags.m_zero = (l_result & 0xFFFFFFFF) == 0;
            p_cpu->m_flags.m_carry = tm_check_bit(l_result, 32);
            break;
        case 1:
            p_cpu->m_flags.m_zero = (l_result & 0xFFFF) == 0;
            p_cpu->m_flags.m_carry = tm_check_bit(l_result, 16);
            break;
        default:
            p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
            p_cpu->m_flags.m_carry = tm_check_bit(l_result, 8);
            break;
    }

    return tm_write_cpu_register(p_cpu, p_cpu->m_param1, l_result & 0xFFFFFFFF);
}


static bool tm_execute_rlc (tm_cpu_t* p_cpu)
{
    // The `RLC` instruction shifts the value stored in the memory data register
    // (MDR) left by one bit. The uppermost bit of the old value is stored in
    // both the carry flag and the lowermost bit of the result.
    // -    If the destination address flag (DA) is set, then the result value
    //      is written to a memory location pointed to by the memory address
    //      register (MAR).
    // -    Otherwise, the result value is stored in the destination register
    //      specified by parameter #1.

    // Shift the MDR's value left one bit. Store the result.
    uint64_t l_result = (p_cpu->m_registers.m_md << 1);

    // Clear the negative, half-carry, overflow and underflow flags.
    p_cpu->m_flags.m_negative = false;
    p_cpu->m_flags.m_half_carry = false;
    p_cpu->m_flags.m_overflow = false;
    p_cpu->m_flags.m_underflow = false;

    if (p_cpu->m_da == true)
    {
        tm_set_bit(l_result, 0, tm_check_bit(l_result, 8));
        p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
        p_cpu->m_flags.m_carry = tm_check_bit(l_result, 8);
        return
            tm_write_byte(p_cpu, p_cpu->m_registers.m_ma, l_result & 0xFF) &&
            tm_cycle_cpu(p_cpu, 1);
    }

    switch (p_cpu->m_param1 & 0b11)
    {
        case 0:
            tm_set_bit(l_result, 0, tm_check_bit(l_result, 32));
            p_cpu->m_flags.m_zero = (l_result & 0xFFFFFFFF) == 0;
            p_cpu->m_flags.m_carry = tm_check_bit(l_result, 32);
            break;
        case 1:
            tm_set_bit(l_result, 0, tm_check_bit(l_result, 16));
            p_cpu->m_flags.m_zero = (l_result & 0xFFFF) == 0;
            p_cpu->m_flags.m_carry = tm_check_bit(l_result, 16);
            break;
        default:
            tm_set_bit(l_result, 0, tm_check_bit(l_result, 8));
            p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
            p_cpu->m_flags.m_carry = tm_check_bit(l_result, 8);
            break;
    }

    return tm_write_cpu_register(p_cpu, p_cpu->m_param1, l_result & 0xFFFFFFFF);

}

static bool tm_execute_rr (tm_cpu_t* p_cpu)
{
    // The `RR` instruction shifts the value stored in the memory data register
    // (MDR) right by one bit. The lowermost bit of the old value is stored in
    // the carry flag, and the uppermost bit is set to the old value of the
    // carry flag.
    // -    If the destination address flag (DA) is set, then the result value
    //      is written to a memory location pointed to by the memory address
    //      register (MAR).
    // -    Otherwise, the result value is stored in the destination register
    //      specified by parameter #1.

    // Keep track of the old value of the MDR, then shift the MDR's value right
    // by one bit and store the result.
    long_t l_old_mdr = p_cpu->m_registers.m_md;
    long_t l_result = (p_cpu->m_registers.m_md >> 1);

    // Keep track of the old state of the carry flag.
    byte_t l_old_carry = p_cpu->m_flags.m_carry;

    // Set the carry flag to bit 0 of the old value.
    p_cpu->m_flags.m_carry = tm_check_bit(l_old_mdr, 0);

    // Clear the negative, half-carry, overflow and underflow flags.
    p_cpu->m_flags.m_negative = false;
    p_cpu->m_flags.m_half_carry = false;
    p_cpu->m_flags.m_overflow = false;
    p_cpu->m_flags.m_underflow = false;

    // If the destination address flag (DA) is set, write the result to memory
    // at the address specified by the memory address register (MAR) and set the
    // appropriate flags.
    if (p_cpu->m_da == true)
    {
        tm_set_bit(l_result, 7, l_old_carry);
        p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
        return
            tm_write_byte(p_cpu, p_cpu->m_registers.m_ma, l_result & 0xFF) &&
            tm_cycle_cpu(p_cpu, 1);
    }

    // Based on the size of the destination register, set the proper bit of the
    // newly-shifted value to the bit's old value. Also, set the zero flag
    // according to the result.
    switch (p_cpu->m_param1 & 0b11)
    {
        case 0:
            tm_set_bit(l_result, 31, l_old_carry);
            p_cpu->m_flags.m_zero = (l_result & 0xFFFFFFFF) == 0;
            break;
        case 1:
            tm_set_bit(l_result, 15, l_old_carry);
            p_cpu->m_flags.m_zero = (l_result & 0xFFFF) == 0;
            break;
        default:
            tm_set_bit(l_result, 7, l_old_carry);
            p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
            break;
    }

    // Write the result back to the destination register.
    return tm_write_cpu_register(p_cpu, p_cpu->m_param1, l_result);
}

static bool tm_execute_rrc (tm_cpu_t* p_cpu)
{
    // The `RRC` instruction shifts the value stored in the memory data register
    // (MDR) right by one bit. The lowermost bit of the old value is stored in
    // both the carry flag and the uppermost bit of the result.
    // -    If the destination address flag (DA) is set, then the result value
    //      is written to a memory location pointed to by the memory address
    //      register (MAR).
    // -    Otherwise, the result value is stored in the destination register
    //      specified by parameter #1.

    // Keep track of the old value of the MDR, then shift the MDR's value right
    // by one bit and store the result.
    long_t l_old_mdr = p_cpu->m_registers.m_md;
    long_t l_result = (p_cpu->m_registers.m_md >> 1);

    // Set the carry flag to bit 0 of the old value.
    p_cpu->m_flags.m_carry = tm_check_bit(l_old_mdr, 0);

    // Clear the negative, half-carry, overflow and underflow flags.
    p_cpu->m_flags.m_negative = false;
    p_cpu->m_flags.m_half_carry = false;
    p_cpu->m_flags.m_overflow = false;
    p_cpu->m_flags.m_underflow = false;

    // If the destination address flag (DA) is set, write the result to memory
    // at the address specified by the memory address register (MAR) and set the
    // appropriate flags.
    if (p_cpu->m_da == true)
    {
        tm_set_bit(l_result, 7, p_cpu->m_flags.m_carry);
        p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
        return
            tm_write_byte(p_cpu, p_cpu->m_registers.m_ma, l_result & 0xFF) &&
            tm_cycle_cpu(p_cpu, 1);
    }

    // Based on the size of the destination register, set the proper bit of the
    // newly-shifted value to the bit's old value. Also, set the zero flag
    // according to the result.
    switch (p_cpu->m_param1 & 0b11)
    {
        case 0:
            tm_set_bit(l_result, 31, p_cpu->m_flags.m_carry);
            p_cpu->m_flags.m_zero = (l_result & 0xFFFFFFFF) == 0;
            break;
        case 1:
            tm_set_bit(l_result, 15, p_cpu->m_flags.m_carry);
            p_cpu->m_flags.m_zero = (l_result & 0xFFFF) == 0;
            break;
        default:
            tm_set_bit(l_result, 7, p_cpu->m_flags.m_carry);
            p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
            break;
    }

    // Write the result back to the destination register.
    return tm_write_cpu_register(p_cpu, p_cpu->m_param1, l_result);
}

static bool tm_execute_bit (tm_cpu_t* p_cpu)
{
    // The `BIT` instruction tests the value of a specific bit of the value
    // stored in the memory data register (MDR) and sets the zero flag if that
    // bit is cleared. The bit to test is specified by an immediate byte after
    // the opcode.

    // Clear the negative flag and set the half-carry flag.
    p_cpu->m_flags.m_negative = false;
    p_cpu->m_flags.m_half_carry = true;

    // Fetch the bit to test from the instruction's immediate byte.
    long_t l_bit = 0;
    if (
        tm_check_readable(p_cpu, p_cpu->m_registers.m_pc, 1) == false ||
        tm_read_byte(p_cpu, p_cpu->m_registers.m_pc, &l_bit) == false ||
        tm_advance_cpu(p_cpu, 1) == false
    )
    {
        return false;
    }

    // If the destination address flag (DA) is set, test the specified bit of
    // the value stored in memory at the address specified by the memory address
    // register (MAR).
    if (p_cpu->m_da == true)
    {
        long_t l_value = 0;
        if (
            tm_read_byte(p_cpu, p_cpu->m_registers.m_ma, &l_value) == false ||
            tm_cycle_cpu(p_cpu, 1) == false
        )
        {
            return false;
        }

        p_cpu->m_flags.m_zero = !tm_check_bit(l_value, (l_bit % 8));
        return true;
    }

    // Test the specified bit of the MDR's value.
    switch (p_cpu->m_param2)
    {
        case 0:
            p_cpu->m_flags.m_zero = !tm_check_bit(p_cpu->m_registers.m_md, (l_bit % 32));
            break;
        case 1:
            p_cpu->m_flags.m_zero = !tm_check_bit(p_cpu->m_registers.m_md, (l_bit % 16));
            break;
        default:
            p_cpu->m_flags.m_zero = !tm_check_bit(p_cpu->m_registers.m_md, (l_bit % 8));
            break;
    }

    return true;
}

static bool tm_execute_set (tm_cpu_t* p_cpu)
{
    // The `SET` instruction sets a specific bit of the value stored in the
    // memory data register (MDR). The bit to set is specified by an immediate
    // byte after the opcode.

    // Clear the negative and half-carry flags, and set the carry flag.
    p_cpu->m_flags.m_negative = false;
    p_cpu->m_flags.m_half_carry = false;
    p_cpu->m_flags.m_carry = true;

    // Fetch the bit to set from the instruction's immediate byte.
    long_t l_bit = 0;
    if (
        tm_check_readable(p_cpu, p_cpu->m_registers.m_pc, 1) == false ||
        tm_read_byte(p_cpu, p_cpu->m_registers.m_pc, &l_bit) == false ||
        tm_advance_cpu(p_cpu, 1) == false
    )
    {
        return false;
    }

    // If the destination address flag (DA) is set, write the result to memory
    // at the address specified by the memory address register (MAR).
    if (p_cpu->m_da == true)
    {
        tm_set_bit(p_cpu->m_registers.m_md, (l_bit % 8), true);
        return
            tm_write_byte(p_cpu, p_cpu->m_registers.m_ma, p_cpu->m_registers.m_md) &&
            tm_cycle_cpu(p_cpu, 1);
    }

    // Set the specified bit of the MDR's value.
    switch (p_cpu->m_param2)
    {
        case 0:
            tm_set_bit(p_cpu->m_registers.m_md, (l_bit % 32), true);
            break;
        case 1:
            tm_set_bit(p_cpu->m_registers.m_md, (l_bit % 16), true);
            break;
        default:
            tm_set_bit(p_cpu->m_registers.m_md, (l_bit % 8), true);
            break;
    }

    // Write the result back to the destination register.
    return tm_write_cpu_register(p_cpu, p_cpu->m_param1, p_cpu->m_registers.m_md);
}

static bool tm_execute_res (tm_cpu_t* p_cpu)
{
    // The `RES` instruction clears a specific bit of the value stored in the
    // memory data register (MDR). The bit to clear is specified by an immediate
    // byte after the opcode.

    // Fetch the bit to clear from the instruction's immediate byte.
    long_t l_bit = 0;
    if (
        tm_check_readable(p_cpu, p_cpu->m_registers.m_pc, 1) == false ||
        tm_read_byte(p_cpu, p_cpu->m_registers.m_pc, &l_bit) == false ||
        tm_advance_cpu(p_cpu, 1) == false
    )
    {
        return false;
    }

    // If the destination address flag (DA) is set, write the result to memory
    // at the address specified by the memory address register (MAR).
    if (p_cpu->m_da == true)
    {
        tm_set_bit(p_cpu->m_registers.m_md, (l_bit % 8), false);
        return
            tm_write_byte(p_cpu, p_cpu->m_registers.m_ma, p_cpu->m_registers.m_md) &&
            tm_cycle_cpu(p_cpu, 1);
    }

    // Clear the specified bit of the MDR's value.
    switch (p_cpu->m_param2)
    {
        case 0:
            tm_set_bit(p_cpu->m_registers.m_md, (l_bit % 32), false);
            break;
        case 1:
            tm_set_bit(p_cpu->m_registers.m_md, (l_bit % 16), false);
            break;
        default:
            tm_set_bit(p_cpu->m_registers.m_md, (l_bit % 8), false);
            break;
    }

    // Write the result back to the destination register.
    return tm_write_cpu_register(p_cpu, p_cpu->m_param1, p_cpu->m_registers.m_md);
}

static bool tm_execute_swap (tm_cpu_t* p_cpu)
{
    // The `SWAP` instruction swaps the upper and lower halves of the value
    // stored in the memory data register (MDR).
    // -    If the destination address flag (DA) is set, then the size of the
    //      halves are one nibble each.
    // -    Otherwise, the size of the halves depend upon the size of the
    //      destination register specified by parameter #1.

    // Clear the negative, half-carry, carry, overflow and underflow flags.
    p_cpu->m_flags.m_negative = false;
    p_cpu->m_flags.m_half_carry = false;
    p_cpu->m_flags.m_carry = false;
    p_cpu->m_flags.m_overflow = false;
    p_cpu->m_flags.m_underflow = false;

    if (p_cpu->m_da == true)
    {
        // If the destination address flag (DA) is set, swap the upper and lower
        // nibbles of the MDR's value.
        long_t l_result = ((p_cpu->m_registers.m_md & 0xF) << 4) |
                          ((p_cpu->m_registers.m_md & 0xF0) >> 4);

        // Set the zero flag based on the result.
        p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;

        // Write the result back to memory at the address specified by the memory
        // address register (MAR).
        return
            tm_write_byte(p_cpu, p_cpu->m_registers.m_ma, l_result & 0xFF) &&
            tm_cycle_cpu(p_cpu, 1);
    }

    // Based on the size of the destination register, swap the upper and lower
    // halves of the MDR's value.
    long_t l_result = 0;
    switch (p_cpu->m_param1 & 0b11)
    {
        case 0:
            l_result = ((p_cpu->m_registers.m_md & 0xFFFF) << 16) |
                       ((p_cpu->m_registers.m_md & 0xFFFF0000) >> 16);
            p_cpu->m_flags.m_zero = (l_result & 0xFFFFFFFF) == 0;
            break;
        case 1:
            l_result = ((p_cpu->m_registers.m_md & 0xFF) << 8) |
                       ((p_cpu->m_registers.m_md & 0xFF00) >> 8);
            p_cpu->m_flags.m_zero = (l_result & 0xFFFF) == 0;
            break;
        default:
            l_result = ((p_cpu->m_registers.m_md & 0xF) << 4) |
                       ((p_cpu->m_registers.m_md & 0xF0) >> 4);
            p_cpu->m_flags.m_zero = (l_result & 0xFF) == 0;
            break;
    }

    // Write the result back to the destination register.
    return tm_write_cpu_register(p_cpu, p_cpu->m_param1, l_result);
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
        p_cpu->m_registers.m_ea = p_address;
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
        p_cpu->m_registers.m_ea = p_address;
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
        p_cpu->m_registers.m_ea = p_address;
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
        p_cpu->m_registers.m_ea = p_address;
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
        p_cpu->m_registers.m_ea = p_address;
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
        p_cpu->m_registers.m_ea = p_address;
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
    // Use this function to cycle the CPU a specific number of times. This is
    // useful for stepping through the CPU's execution cycle without advancing
    // the program counter.
    //
    // Call the `m_cycle` function pointer for each cycle. If the function
    // returns false, then set the CPU's error flag and return false.

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
    // Use this function instead of the `tm_cycle_cpu` function if you need to
    // advance the program counter by the same cycle count.

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
        // The CPU should not run at all if the stop flag is set.
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

        // 2a. Reset the instruction and parameter registers to zero.
        p_cpu->m_inst   = 0;
        p_cpu->m_param1 = 0;
        p_cpu->m_param2 = 0;

        // 2b.  An instruction's operation code (opcode) contains important
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
            case 0x00:  l_good = tm_execute_nop(p_cpu);                                         break;
            case 0x01:  l_good = tm_execute_stop(p_cpu);                                        break;
            case 0x02:  l_good = tm_execute_halt(p_cpu);                                        break;
            case 0x03:  l_good = tm_execute_sec(p_cpu);                                         break;
            case 0x04:  l_good = tm_execute_cec(p_cpu);                                         break;
            case 0x05:  l_good = tm_execute_di(p_cpu);                                          break;
            case 0x06:  l_good = tm_execute_ei(p_cpu);                                          break;
            case 0x07:  l_good = tm_execute_daa(p_cpu);                                         break;
            case 0x08:  l_good = tm_execute_cpl(p_cpu);                                         break;
            case 0x09:  l_good = tm_execute_cpw(p_cpu);                                         break;
            case 0x0A:  l_good = tm_execute_cpb(p_cpu);                                         break;
            case 0x0B:  l_good = tm_execute_scf(p_cpu);                                         break;
            case 0x0C:  l_good = tm_execute_ccf(p_cpu);                                         break;

            case 0x10:  l_good = tm_fetch_reg_imm(p_cpu)        && tm_execute_ld(p_cpu);        break;
            case 0x11:  l_good = tm_fetch_reg_addr32(p_cpu)     && tm_execute_ld(p_cpu);        break;
            case 0x12:  l_good = tm_fetch_reg_regptr32(p_cpu)   && tm_execute_ld(p_cpu);        break;
            case 0x13:  l_good = tm_fetch_reg_addr16(p_cpu)     && tm_execute_ld(p_cpu);        break;
            case 0x15:  l_good = tm_fetch_reg_addr8(p_cpu)      && tm_execute_ld(p_cpu);        break;
            case 0x17:  l_good = tm_fetch_addr32_reg(p_cpu)     && tm_execute_st(p_cpu);        break;
            case 0x18:  l_good = tm_fetch_regptr32_reg(p_cpu)   && tm_execute_st(p_cpu);        break;
            case 0x19:  l_good = tm_fetch_addr16_reg(p_cpu)     && tm_execute_st(p_cpu);        break;
            case 0x1B:  l_good = tm_fetch_addr8_reg(p_cpu)      && tm_execute_st(p_cpu);        break;
            case 0x1D:  l_good = tm_fetch_reg(p_cpu, true)      && tm_execute_mv(p_cpu);        break;
            case 0x1E:  l_good = tm_fetch_reg(p_cpu, true)      && tm_execute_push(p_cpu);      break;
            case 0x1F:  l_good = tm_execute_pop(p_cpu);                                         break;

            case 0x20:  l_good = tm_fetch_addr32(p_cpu, false)  && tm_execute_jmp(p_cpu);       break;
            case 0x21:  l_good = tm_fetch_regptr32(p_cpu, false)&& tm_execute_jmp(p_cpu);       break;
            case 0x22:  l_good = tm_fetch_imm16(p_cpu)          && tm_execute_jpb(p_cpu);       break;
            case 0x23:  l_good = tm_fetch_addr32(p_cpu, false)  && tm_execute_call(p_cpu);      break;
            case 0x24:  l_good = tm_execute_rst(p_cpu);                                         break;
            case 0x25:  l_good = tm_execute_ret(p_cpu);                                         break;
            case 0x26:  l_good = tm_execute_reti(p_cpu);                                        break;
            case 0x27:  
            case 0xFF:  l_good = tm_execute_jps(p_cpu);                                         break;

            case 0x30:  l_good = tm_fetch_reg(p_cpu, false)     && tm_execute_inc(p_cpu);       break;
            case 0x31:  l_good = tm_fetch_regptr32(p_cpu, true) && tm_execute_inc(p_cpu);       break;
            case 0x32:  l_good = tm_fetch_reg(p_cpu, false)     && tm_execute_dec(p_cpu);       break;
            case 0x33:  l_good = tm_fetch_regptr32(p_cpu, true) && tm_execute_dec(p_cpu);       break;
            case 0x34:  l_good = tm_fetch_reg_imm(p_cpu)        && tm_execute_add(p_cpu, false);break;
            case 0x35:  l_good = tm_fetch_reg(p_cpu, true)      && tm_execute_add(p_cpu, false);break;
            case 0x36:  l_good = tm_fetch_reg_regptr32(p_cpu)   && tm_execute_add(p_cpu, false);break;
            case 0x37:  l_good = tm_fetch_reg_imm(p_cpu)        && tm_execute_add(p_cpu, true); break;
            case 0x38:  l_good = tm_fetch_reg(p_cpu, true)      && tm_execute_add(p_cpu, true); break;
            case 0x39:  l_good = tm_fetch_reg_regptr32(p_cpu)   && tm_execute_add(p_cpu, true); break;
            case 0x3A:  l_good = tm_fetch_reg_imm(p_cpu)        && tm_execute_sub(p_cpu, false);break;
            case 0x3B:  l_good = tm_fetch_reg(p_cpu, true)      && tm_execute_sub(p_cpu, false);break;
            case 0x3C:  l_good = tm_fetch_reg_regptr32(p_cpu)   && tm_execute_sub(p_cpu, false);break;
            case 0x3D:  l_good = tm_fetch_reg_imm(p_cpu)        && tm_execute_sub(p_cpu, true); break;
            case 0x3E:  l_good = tm_fetch_reg(p_cpu, true)      && tm_execute_sub(p_cpu, true); break;
            case 0x3F:  l_good = tm_fetch_reg_regptr32(p_cpu)   && tm_execute_sub(p_cpu, true); break;

            case 0x40:  l_good = tm_fetch_reg_imm(p_cpu)        && tm_execute_and(p_cpu);       break;
            case 0x41:  l_good = tm_fetch_reg(p_cpu, true)      && tm_execute_and(p_cpu);       break;
            case 0x42:  l_good = tm_fetch_reg_regptr32(p_cpu)   && tm_execute_and(p_cpu);       break;
            case 0x43:  l_good = tm_fetch_reg_imm(p_cpu)        && tm_execute_or(p_cpu);        break;
            case 0x44:  l_good = tm_fetch_reg(p_cpu, true)      && tm_execute_or(p_cpu);        break;
            case 0x45:  l_good = tm_fetch_reg_regptr32(p_cpu)   && tm_execute_or(p_cpu);        break;
            case 0x46:  l_good = tm_fetch_reg_imm(p_cpu)        && tm_execute_xor(p_cpu);       break;
            case 0x47:  l_good = tm_fetch_reg(p_cpu, true)      && tm_execute_xor(p_cpu);       break;
            case 0x48:  l_good = tm_fetch_reg_regptr32(p_cpu)   && tm_execute_xor(p_cpu);       break;
            case 0x49:  l_good = tm_fetch_reg_imm(p_cpu)        && tm_execute_cmp(p_cpu);       break;
            case 0x4A:  l_good = tm_fetch_reg(p_cpu, true)      && tm_execute_cmp(p_cpu);       break;
            case 0x4B:  l_good = tm_fetch_reg_regptr32(p_cpu)   && tm_execute_cmp(p_cpu);       break;

            case 0x50:  l_good = tm_fetch_reg(p_cpu, false)     && tm_execute_sla(p_cpu);       break;
            case 0x51:  l_good = tm_fetch_regptr32(p_cpu, true) && tm_execute_sla(p_cpu);       break;
            case 0x52:  l_good = tm_fetch_reg(p_cpu, false)     && tm_execute_sra(p_cpu);       break;
            case 0x53:  l_good = tm_fetch_regptr32(p_cpu, true) && tm_execute_sra(p_cpu);       break;
            case 0x54:  l_good = tm_fetch_reg(p_cpu, false)     && tm_execute_srl(p_cpu);       break;
            case 0x55:  l_good = tm_fetch_regptr32(p_cpu, true) && tm_execute_srl(p_cpu);       break;
            case 0x56:  l_good = tm_fetch_reg(p_cpu, false)     && tm_execute_rl(p_cpu);        break;
            case 0x57:  l_good = tm_fetch_regptr32(p_cpu, true) && tm_execute_rl(p_cpu);        break;
            case 0x58:  l_good = tm_fetch_reg(p_cpu, false)     && tm_execute_rlc(p_cpu);       break;
            case 0x59:  l_good = tm_fetch_regptr32(p_cpu, true) && tm_execute_rlc(p_cpu);       break;
            case 0x5A:  l_good = tm_fetch_reg(p_cpu, false)     && tm_execute_rr(p_cpu);        break;
            case 0x5B:  l_good = tm_fetch_regptr32(p_cpu, true) && tm_execute_rr(p_cpu);        break;
            case 0x5C:  l_good = tm_fetch_reg(p_cpu, false)     && tm_execute_rrc(p_cpu);       break;
            case 0x5D:  l_good = tm_fetch_regptr32(p_cpu, true) && tm_execute_rrc(p_cpu);       break;

            case 0x60:  l_good = tm_fetch_reg(p_cpu, true)      && tm_execute_bit(p_cpu);       break;
            case 0x61:  l_good = tm_fetch_regptr32(p_cpu, true) && tm_execute_bit(p_cpu);       break;
            case 0x62:  l_good = tm_fetch_reg(p_cpu, true)      && tm_execute_set(p_cpu);       break;
            case 0x63:  l_good = tm_fetch_regptr32(p_cpu, true) && tm_execute_set(p_cpu);       break;
            case 0x64:  l_good = tm_fetch_reg(p_cpu, true)      && tm_execute_res(p_cpu);       break;
            case 0x65:  l_good = tm_fetch_regptr32(p_cpu, true) && tm_execute_res(p_cpu);       break;
            case 0x66:  l_good = tm_fetch_reg(p_cpu, false)     && tm_execute_swap(p_cpu);      break;
            case 0x67:  l_good = tm_fetch_regptr32(p_cpu, true) && tm_execute_swap(p_cpu);      break;

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
        // If the CPU is halted, cycle the CPU once and check if an interrupt
        // has been requested. If an interrupt has been requested, clear the
        // halt flag.
        tm_cycle_cpu(p_cpu, 1);
        if (p_cpu->m_registers.m_if != 0)
        {
            p_cpu->m_flags.m_halt = false;
        }
    }

    // If the CPU's interrupt master enable flag is set, handle any pending
    // interrupts if their respective interrupt enable bit is set.
    if (p_cpu->m_ime == true)
    {
        tm_handle_interrupts(p_cpu);
        p_cpu->m_enable_ime = false;
    }

    // If the `EI` instruction set the `enable_ime` flag, enable the interrupt
    // master flag.
    if (p_cpu->m_enable_ime == true)
    {
        p_cpu->m_ime = true;
    }

    return true;
}

/* Public Functions - Error Checking ******************************************/

bool tm_has_error (tm_cpu_t* p_cpu)
{
    tm_assert(p_cpu != nullptr);
    return
        p_cpu->m_registers.m_ec != TM_ERROR_OK &&
        p_cpu->m_flags.m_stop == true;
}

const char* tm_get_error (tm_cpu_t* p_cpu)
{
    tm_assert(p_cpu != nullptr);

    switch (p_cpu->m_registers.m_ec)
    {
        case TM_ERROR_OK:
            snprintf(p_cpu->m_error_string, TM_ERROR_STRLEN,
                "The program exited successfully.");
            break;
        case TM_ERROR_HARDWARE:
            snprintf(p_cpu->m_error_string, TM_ERROR_STRLEN,
                "A hardware error occurred.");
            break;
        case TM_ERROR_INVALID_OPCODE:
            snprintf(p_cpu->m_error_string, TM_ERROR_STRLEN,
                "An invalid opcode 0x%02X was encountered at address $%08X.",
                    p_cpu->m_inst, p_cpu->m_registers.m_ia);
            break;
        case TM_ERROR_BUS_READ:
            snprintf(p_cpu->m_error_string, TM_ERROR_STRLEN,
                "A hardware error occured while reading from the bus at address $%08X.",
                    p_cpu->m_registers.m_ea);
            break;
        case TM_ERROR_BUS_WRITE:
            snprintf(p_cpu->m_error_string, TM_ERROR_STRLEN,
                "A hardware error occured while writing to the bus at address $%08X.",
                    p_cpu->m_registers.m_ea);
            break;
        case TM_ERROR_READ_ACCESS_VIOLATION:
            snprintf(p_cpu->m_error_string, TM_ERROR_STRLEN,
                "The instruction 0x%02X at address $%08X attempted to read from non-readable memory address $%08X.",
                    p_cpu->m_inst, p_cpu->m_registers.m_ia, p_cpu->m_registers.m_ea);
            break;
        case TM_ERROR_WRITE_ACCESS_VIOLATION:
            snprintf(p_cpu->m_error_string, TM_ERROR_STRLEN,
                "The instruction 0x%02X at address $%08X attempted to write to non-writable memory address $%08X.",
                    p_cpu->m_inst, p_cpu->m_registers.m_ia, p_cpu->m_registers.m_ea);
            break;
        case TM_ERROR_EXECUTE_ACCESS_VIOLATION:
            snprintf(p_cpu->m_error_string, TM_ERROR_STRLEN,
                "Attempted to execute non-executable memory at address $%08X.",
                    p_cpu->m_registers.m_ea);
            break;
        case TM_ERROR_DATA_STACK_OVERFLOW:
            snprintf(p_cpu->m_error_string, TM_ERROR_STRLEN,
                "The data stack overflowed while executing the instruction 0x%02X at address $%08X.",
                    p_cpu->m_inst, p_cpu->m_registers.m_ia);
            break;
        case TM_ERROR_DATA_STACK_UNDERFLOW:
            snprintf(p_cpu->m_error_string, TM_ERROR_STRLEN,
                "The data stack underflowed while executing the instruction 0x%02X at address $%08X.",
                    p_cpu->m_inst, p_cpu->m_registers.m_ia);
            break;
        case TM_ERROR_CALL_STACK_OVERFLOW:
            snprintf(p_cpu->m_error_string, TM_ERROR_STRLEN,
                "The call stack overflowed while executing the instruction 0x%02X at address $%08X.",
                    p_cpu->m_inst, p_cpu->m_registers.m_ia);
            break;
        case TM_ERROR_CALL_STACK_UNDERFLOW:
            snprintf(p_cpu->m_error_string, TM_ERROR_STRLEN,
                "The call stack underflowed while executing the instruction 0x%02X at address $%08X.",
                    p_cpu->m_inst, p_cpu->m_registers.m_ia);
            break;
        default:
            snprintf(p_cpu->m_error_string, TM_ERROR_STRLEN,
                "The program has stopped with error code 0x%02X at address $%08X.",
                    p_cpu->m_registers.m_ec, p_cpu->m_registers.m_ia);
            break;
    }

    return p_cpu->m_error_string;
}
