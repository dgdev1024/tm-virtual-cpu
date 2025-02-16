# TM CPU Specification

The following is a specification for the TM virtual CPU.

## Glossary

| Term                  | Definition                                                                |
|-----------------------|---------------------------------------------------------------------------|
| Bit                   | A value that is either `0` or `1`. The basic building block of integers.  |
| Long                  | A 32-bit unsigned integer.                                                |
| Word                  | A 16-bit unsigned integer.                                                |
| Byte                  | An 8-bit unsigned integer.                                                |
| Nibble                | A 4-bit unsigned integer - one half of a byte.                            |
| Hi Word               | The upper word of a long (`0xXXXX0000`).                                  |
| Lo Word               | The lower word of a long (`0x0000XXXX`).                                  |
| Hi Byte               | The upper byte of a word (`0xXX00`).                                      |
| Lo Byte               | The lower byte of a word (`0x00XX`).                                      |
| Hi Nibble             | The upper nibble in a byte (`0xX0`).                                      |
| Lo Nibble             | The lower nibble in a byte (`0x0X`).                                      |
| Opcode                | Operation code; describes an instruction to be executed by the CPU.       |
| Restart Vector        | A special subroutine called by the `RST` instruction.                     |
| Interrupt             | An event requested by hardware, for the CPU to handle immediately.        |
| Interrupt Vector      | A special subroutine called by the CPU in response to interrupts.         |

## General Notes

- Address Bus: 32-bit
- Memory Unit Size: 8-bit
- Opcode Size: 16-bit
- Byte Order: Little-Endian

## Memory Map

| Start Address | End Address   | Description                                                   |
|---------------|---------------|---------------------------------------------------------------|
| `$00000000`   | `$7FFFFFFF`   | Read-only memory (ROM) space.                                 |
| `$00000000`   | `$00000FFF`   | Reserved for program metadata.                                |
| `$00001000`   | `$00001FFF`   | Reserved for restart vector subroutines.                      |
| `$00002000`   | `$00002FFF`   | Reserved for interrupt vector subroutines.                    |
| `$00003000`   | `$7FFFFFFF`   | Reserved for program instructions and data.                   |
| `$80000000`   | `$FFFFFFFF`   | Random-access memory (RAM) space.                             |
| `$C0000000`   | `$FFFCFFFF`   | Executable random-access memory (XRAM) space.                 |
| `$FFFD0000`   | `$FFFDFFFF`   | Reserved for a general-purpose memory stack.                  |
| `$FFFE0000`   | `$FFFEFFFF`   | Reserved for a call stack for storing return addresses.       |
| `$FFFF0000`   | `$FFFFFFFF`   | Quick random-access memory (QRAM) space.                      |
| `$FFFFFF00`   | `$FFFFFFFF`   | Reserved for hardware-specific registers.                     |

## Program Metadata

The first `$1000` bytes of the program is reserved for metadata, which is information describing the
program. The table below indicates the metadata which is expected by the TM CPU. Additional metadata
may be requested by any emulator/hardware powered by the TM CPU.

| Address       | Size          | Description                                                   |
|---------------|---------------|---------------------------------------------------------------|
| `$0000`       | 4 Bytes       | TM Program Magic Number - `TM08` - `0x38304D54`.              |
| `$0004`       | 123 Bytes     | Program Name - ASCII characters only.                         |
| `$007F`       | 1 Byte        | Program Name Null Terminator.                                 |
| `$0080`       | 127 Bytes     | Program Author - ASCII characters only.                       |
| `$015F`       | 1 Byte        | Program Author Null Terminator.                               |
| `$0160`       | 4 Bytes       | Program Expected ROM Size, in Bytes.                          |

## CPU Registers

### General-Purpose Registers

| Long      | Lo Word   | Hi Byte   | Lo Byte   | Notes                                         |
|-----------|-----------|-----------|-----------|-----------------------------------------------|
| `A`       | `AW`      | `AH`      | `AL`      | Accumulator Register                          |
| `B`       | `BW`      | `BH`      | `BL`      | General Register                              |
| `C`       | `CW`      | `CH`      | `CL`      | General Register                              |
| `D`       | `DW`      | `DH`      | `DL`      | General Register                              |

#### Notes

- All general purpose registers have an initial value of `0x00`.
- Given register `X`...
    - `XW` is the lo word of register `X`.
    - `XH` is the hi byte of word register `XW`.
    - `XL` is the lo byte of word register `XW`.

### Special-Purpose Registers

| Register              | Description               | Size              | Initial Value         |
|-----------------------|---------------------------|-------------------|-----------------------|
| `F`                   | Flags Register            | Byte              | `0x00`                |
| `IME`                 | Interrupt Master Enable   | Bit               | `0b0`                 |
| `EC`                  | Error Code                | Byte              | `0x00`                |
| `PC`                  | Program Counter           | Long              | `0x00003000`          |
| `SP`                  | Stack Pointer             | Word              | `0xFFFF`              |
| `RP`                  | Return Pointer            | Word              | `0xFFFF`              |
| `IA`                  | Inst. Address Register    | Long              | `0x00000000`          |
| `MA`                  | Memory Address Register   | Long              | `0x00000000`          |
| `MD`                  | Memory Data Register      | Long              | `0x00000000`          |
| `CI`                  | Current Instruction Reg.  | Word              | `0xFFFF`              |
| `DA`                  | Destination Address Flag  | Bit               | `0b0`                 |

#### Notes

- The `SP` register is a word address relative to the start of the general-purpose stack's starting
  address, `$FFFD0000`.
- The `RP` register is a word address relative to the start of the call stack's starting address,
  `$FFFE0000`.
- When data is pushed into the stack, the appropriate stack pointer is first decremented by 4, then
  the data is written to the stack at the stack pointer's new position.
- When data is popped from the stack, the data is read from the stack at the stack pointer's current
  position, then the stack pointer is incremented by 4.

## Flags Register

The flags register `F` is a special-purpose register indicating the current state of the CPU. Its
bits, listed below from left-to-right, indicate the following:

- Bit 7: `Z` - Zero Flag
    - Set if the result of the previous instruction is zero.
- Bit 6: `N` - Negative Flag
    - Also called the Subtraction Flag.
    - Set if the previous instruction was, or involved, a subtraction.
- Bit 5: `H` - Half-Carry Flag
    - Set if there was a carry in the lower half of the result of the previous instruction.
        - For instructions affecting long register `A`, check for carry in the lo word thereof (word register `AW`).
        - For instructions affecting word register `AW`, check for carry in the lo byte thereof (byte register `AL`).
        - For instructions affecting byte register `AL`, check for carry in the lo nibble thereof.
- Bit 4: `C` - Carry Flag
    - Set if there was a carry in the result of the previous instruction.
- Bit 3: `O` - Overflow Flag
    - Set if the result of the previous instruction would overflow the target accumulator register,
      causing it to wrap around to zero.
- Bit 2: `U` - Underflow Flag
    - Set if the result of the previous instruction would underflow the target accumulator register,
      causing it to wrap around to its maximum value.
- Bit 1: `L` - Halt Flag
    - Set to halt the CPU.
    - The CPU will not execute instructions while in this state.
    - Cleared automatically when an interrupt is pending.
- Bit 0: `S` - Stop Flag
    - Set to stop the CPU.
    - The CPU will not execute instructions or any handlers in this state.
    - Check the `EC` register for an error code if this state is encountered.

## Execution Conditions

Certain instructions in the TM's instruction set, namely the control transfer instructions, can
require the fulfillment of an **execution condition**, in the form of whether or not a certain CPU
flag is set, in order to execute. The execution conditions which can be used are listed below:

| Condition     | Description                                                                   |
|---------------|-------------------------------------------------------------------------------|
| `N`           | No condition needs to be fulfilled to execute this instruction.               |
| `CS`          | The instruction will executed only if the `C` flag is set.                    |
| `CC`          | The instruction will executed only if the `C` flag is clear.                  |
| `ZS`          | The instruction will executed only if the `Z` flag is set.                    |
| `ZC`          | The instruction will executed only if the `Z` flag is clear.                  |
| `OS`          | The instruction will executed only if the `O` flag is set.                    |
| `US`          | The instruction will executed only if the `U` flag is set.                    |

## Instruction Set

### General Instructions

| Opcode    | Mnemonic      | Flags         | Description                                       |
|-----------|---------------|---------------|---------------------------------------------------|
| `0x0000`  | `NOP`         | `--------`    | Does nothing.                                     |
| `0x0100`  | `STOP`        | `-------1`    | Sets the CPU's stop flag, stopping execution.     |
| `0x0200`  | `HALT`        | `------1-`    | Sets the CPU's halt flag.                         |
| `0x03XX`  | `SEC XX`      | `--------`    | Sets the CPU's `EC` register to `XX`.             |
| `0x0400`  | `CEC`         | `--------`    | Clears the CPU's `EC` register. Same as `SEC 0`.  |
| `0x0500`  | `DI`          | `--------`    | Clears the CPU's `IME` register.                  |
| `0x0600`  | `EI`          | `--------`    | Sets the CPU's `IME` register.                    |
| `0x0700`  | `DAA`         | `?-0???--`    | Decimal-adjusts byte accumulator register `AL`.   |
| `0x0800`  | `CPL`         | `-11-----`    | Compliments long accumulator register `A`.        |
| `0x0900`  | `CPW`         | `-11-----`    | Compliments word accumulator register `AW`.       |
| `0x0A00`  | `CPB`         | `-11-----`    | Compliments byte accumulator register `AL`.       |
| `0x0B00`  | `SCF`         | `-00100--`    | Sets the carry flag.                              |
| `0x0C00`  | `CCF`         | `-00?00--`    | Compliments the carry flag.                       |

### Data Transfer Instructions

| Opcode    | Mnemonic          | Description                                                           |
|-----------|-------------------|-----------------------------------------------------------------------|
| `0x10X0`  | `LD X, I`         | Loads immediate value `I` into register `X`.                          |
| `0x11X0`  | `LD X, [A32]`     | Loads value in memory at address `A32` into register `X`.             |
| `0x12XY`  | `LD X, [Y]`       | Loads value pointed to by register `Y` into register `X`.             |
| `0x13X0`  | `LDQ X, [A16]`    | Loads value in QRAM at relative address `A16` into register `X`.      |
| `0x15X0`  | `LDH X, [A8]`     | Loads hardware register at relative address `A8` into register `X`.   |
| `0x170Y`  | `ST [A32], Y`     | Stores register `Y` in memory at address `A32`.                       |
| `0x18XY`  | `ST [X], Y`       | Stores register `Y` in memory location pointed to by register `X`.    |
| `0x190Y`  | `STQ [A16], Y`    | Stores register `Y` in QRAM at relative address `A16`.                |
| `0x1B0Y`  | `STH [A8], Y`     | Stores register `Y` in hardware register at relative address `A8`.    |
| `0x1DXY`  | `MV X, Y`         | Copies (moves) register `Y` into register `X`.                        |
| `0x1E0X`  | `PUSH X`          | Pushes register `X` into the general stack.                           |
| `0x1FX0`  | `POP X`           | Pops a value from the general stack, then loads it into register `X`. |

### Control Transfer Instructions

| Opcode    | Mnemonic          | Description                                                               |
|-----------|-------------------|---------------------------------------------------------------------------|
| `0x20X0`  | `JMP X, [A32]`    | Provided condition `X`, moves `PC` to address `A32`.                      |
| `0x21XY`  | `JMP X, [Y]`      | Provided condition `X`, moves `PC` to address pointed to by register `Y`. |
| `0x22X0`  | `JPB X, S16`      | Provided condition `X`, moves `PC` by signed offset `S16`.                |
| `0x23X0`  | `CALL X, [A32]`   | Provided condition `X`, calls subroutine at address `A32`.                |
| `0x24X0`  | `RST X`           | Calls restart vector `X`, starting at address `$00001X00`.                |
| `0x25X0`  | `RET X`           | Provided condition `X`, returns from current subroutine.                  |
| `0x2600`  | `RETI`            | Sets the CPU's `IME` register, then returns from current subroutine.      |
| `0xFFFF`  | `JPS`             | Moves `PC` to its original starting address, `$00003000`.                 |

### Arithmetic Instructions

| Opcode    | Mnemonic      | Flags         | Description                                                                           |
|-----------|---------------|---------------|---------------------------------------------------------------------------------------|
| `0x30X0`  | `INC X`       | `?0?-----`    | Increments register `X`.                                                              |
| `0x310Y`  | `INC [Y]`     | `?0?-----`    | Increments value pointed to by register `Y`.                                          |
| `0x32X0`  | `DEC X`       | `?1?-----`    | Decrements register `X`.                                                              |
| `0x330Y`  | `DEC [Y]`     | `?1?-----`    | Decrements value pointed to by register `Y`.                                          |
| `0x34X0`  | `ADD X, I`    | `?0???0--`    | Adds immediate value `I` to accumulator register `X`.                                 |
| `0x35XY`  | `ADD X, Y`    | `?0???0--`    | Adds register `Y` to accumulator register `X`.                                        |
| `0x36XY`  | `ADD X, [Y]`  | `?0???0--`    | Adds value pointed to by register `Y` to accumulator register `X`.                    |
| `0x37X0`  | `ADC X, I`    | `?0???0--`    | Adds carry and immediate value `I` to accumulator register `X`.                       |
| `0x38XY`  | `ADC X, Y`    | `?0???0--`    | Adds carry and register `Y` to accumulator register `X`.                              |
| `0x39XY`  | `ADC X, [Y]`  | `?0???0--`    | Adds carry and value pointed to by register `Y` to accumulator register `X`.          |
| `0x3AX0`  | `SUB X, I`    | `?1??0?--`    | Adds immediate value `I` to accumulator register `X`.                                 |
| `0x3BXY`  | `SUB X, Y`    | `?1??0?--`    | Adds register `Y` to accumulator register `X`.                                        |
| `0x3CXY`  | `SUB X, [Y]`  | `?1??0?--`    | Adds value pointed to by register `Y` to accumulator register `X`.                    |
| `0x3DX0`  | `SBC X, I`    | `?1??0?--`    | Subtracts carry and immediate value `I` from accumulator register `X`.                |
| `0x3EXY`  | `SBC X, Y`    | `?1??0?--`    | Subtracts carry and register `Y` from accumulator register `X`.                       |
| `0x3FXY`  | `SBC X, [Y]`  | `?1??0?--`    | Subtracts carry and value pointed to by register `Y` from accumulator register `X`.   |

### Bitwise and Comparison Instructions

| Opcode    | Mnemonic      | Flags         | Description                                                                           |
|-----------|---------------|---------------|---------------------------------------------------------------------------------------|
| `0x40X0`  | `AND X, I`    | `?01000--`    | Bitwise ANDs accumulator register `X` and immediate value `I`.                        |
| `0x41X0`  | `AND X, Y`    | `?01000--`    | Bitwise ANDs accumulator register `X` and register `Y`.                               |
| `0x42XY`  | `AND X, [Y]`  | `?01000--`    | Bitwise ANDs accumulator register `X` and value pointed to by register `Y`.           |
| `0x43X0`  | `OR X, I`     | `?00000--`    | Bitwise ORs accumulator register `X` and immediate value `I`.                         |
| `0x44X0`  | `OR X, Y`     | `?00000--`    | Bitwise ORs accumulator register `X` and register `Y`.                                |
| `0x45XY`  | `OR X, [Y]`   | `?00000--`    | Bitwise ORs accumulator register `X` and value pointed to by register `Y`.            |
| `0x46X0`  | `XOR X, I`    | `?00000--`    | Bitwise XORs accumulator register `X` and immediate value `I`.                        |
| `0x47X0`  | `XOR X, Y`    | `?00000--`    | Bitwise XORs accumulator register `X` and register `Y`.                               |
| `0x48XY`  | `XOR X, [Y]`  | `?00000--`    | Bitwise XORs accumulator register `X` and value pointed to by register `Y`.           |
| `0x49X0`  | `CMP X, I`    | `?1??0?--`    | Compares accumulator register `X` and immediate value `I`.                            |
| `0x4AX0`  | `CMP X, Y`    | `?1??0?--`    | Compares accumulator register `X` and register `Y`.                                   |
| `0x4BX0`  | `CMP X, [Y]`  | `?1??0?--`    | Compares accumulator register `X` and value pointed to by register `Y`.               |

### Bit Shifting Instructions

| Opcode    | Mnemonic      | Flags         | Description                                                                           |
|-----------|---------------|---------------|---------------------------------------------------------------------------------------|
| `0x50X0`  | `SLA X`       | `?00?00--`    | Arithmetically left-shifts register `X`.                                              |
| `0x510X`  | `SLA [X]`     | `?00?00--`    | Arithmetically left-shifts value pointed to by register `X`.                          |
| `0x52X0`  | `SRA X`       | `?00?00--`    | Arithmetically right-shifts register `X`.                                             |
| `0x530X`  | `SRA [X]`     | `?00?00--`    | Arithmetically right-shifts value pointed to by register `X`.                         |
| `0x54X0`  | `SRL X`       | `?00?00--`    | Logically right-shifts register `X`.                                                  |
| `0x550X`  | `SRL [X]`     | `?00?00--`    | Logically right-shifts value pointed to by register `X`.                              |
| `0x56X0`  | `RL X`        | `?00?00--`    | Rotates register `X` left one bit, through carry.                                     |
| `0x570X`  | `RL [X]`      | `?00?00--`    | Rotates value pointed to by register `X` left one bit, through carry.                 |
| `0x58X0`  | `RLC X`       | `?00?00--`    | Rotates register `X` left one bit.                                                    |
| `0x590X`  | `RLC [X]`     | `?00?00--`    | Rotates value pointed to by register `X` left one bit.                                | 
| `0x5AX0`  | `RR X`        | `?00?00--`    | Rotates register `X` right one bit, through carry.                                    |
| `0x5B0X`  | `RR [X]`      | `?00?00--`    | Rotates value pointed to by register `X` right one bit, through carry.                |
| `0x5CX0`  | `RRC X`       | `?00?00--`    | Rotates register `X` right one bit.                                                   |
| `0x5D0X`  | `RRC [X]`     | `?00?00--`    | Rotates value pointed to by register `X` right one bit.                               | 

### Bit Checking Instructions

| Opcode    | Mnemonic      | Flags         | Description                                                                           |
|-----------|---------------|---------------|---------------------------------------------------------------------------------------|
| `0x600Y`  | `BIT X, Y`    | `?01-----`    | Checks the state of bit `X` of register `Y`.                                          |
| `0x610Y`  | `BIT X, [Y]`  | `?01-----`    | Checks the state of bit `X` of value pointed to by register `Y`.                      |
| `0x620Y`  | `SET X, Y`    | `-001----`    | Sets bit `X` of register `Y`.                                                         |
| `0x630Y`  | `SET X, [Y]`  | `-001----`    | Sets bit `X` of value pointed to by register `Y`.                                     |
| `0x640Y`  | `RES X, Y`    | `--------`    | Clears bit `X` of register `Y`.                                                       |
| `0x650Y`  | `RES X, [Y]`  | `--------`    | Clears bit `X` of value pointed to by register `Y`.                                   |
| `0x66X0`  | `SWAP X`      | `?00000--`    | Swaps the upper and lower half of register `X`.                                       |
| `0x670X`  | `SWAP [X]`    | `?00000--`    | Swaps the upper and lower half of value pointed to by register `X`.                   |

#### Instruction Flags

- Legend
    - Shown in the following order: `ZNHCOUHS`
    - `0`: Clear the flag.
    - `1`: Set the flag.
    - `?`: Clear or set the flag depending on the instruction's result.
    - `-`: Flag not affected.

- `DAA`
    - `Z`: Set if the adjustment result is zero; clear otherwise.
    - `H`: Clear.
    - `C`: Set if the adjustment result...
        - ...underflows the target accumulator register if `N` is set; clear otherwise.
        - ...overflows the target accumulator register if `N` is clear; clear otherwise.
    - `O`: Set if `N` is clear and this instruction sets `C`.
    - `U`: Set if `N` is set and this instruction sets `C`.
- `CPL`, `CPW`, `CPB`
    - `N` and `H`: Set.
- `SCF`
    - `N`, `H`, `O` and `U`: Clear.
    - `C`: Set.
- `CCF`
    - `N`, `H`, `O` and `U`: Clear.
    - `C`: Clear if set; set if clear.
- `INC`
    - `Z`: Set if the result wraps around to zero.
    - `N`: Clear.
    - `H`: For byte and word registers, set if the lower half of the result overflows.
- `ADD`, `ADC`
    - `Z`: Set if the result wraps around to zero.
    - `N`: Clear.
    - `H`: Set if the lower half of the result overflows.
    - `C` and `O`: Set if the result overflows.
    - `U`: Clear.
- `DEC`
    - `Z`: Set if the result is zero.
    - `N`: Set.
    - `H`: Set if the lower half of the result underflows.
- `SUB`, `SBC`
    - `Z`: Set if the result is zero.
    - `N`: Set.
    - `H`: Set if the lower half of the result underflows.
    - `C` and `U`: Set if the result underflows.
    - `O`: Clear.
- `AND`
    - `Z`: Set if the result is zero.
    - `N`, `C`, `O` and `U`: Clear.
    - `H`: Set.
- `OR`, `XOR`, `SWAP`
    - `Z`: Set if the result is zero.
    - `N`, `H`, `C`, `O` and `U`: Clear.
- `CMP`
    - `Z`: Set if the result is zero (the compared values are the same).
    - `N`: Set.
    - `H`: Set if the lower half of the difference underflows.
    - `C` and `U`: Set if the difference underflows.
    - `O`: Clear.
- `SLA`, `RL`, `RLC`
    - `Z`: Set if the result is zero.
    - `N` and `H`: Clear.
    - `C`: Set to bit 7 before shifting / rotating.
- `SRA`, `SRL`, `RR`, `RRC`
    - `Z`: Set if the result is zero.
    - `N` and `H`: Clear.
    - `C`: Set to bit 0 before shifting / rotating.
- `BIT`
    - `Z`: Set if the bit checked is clear.
    - `N`: Clear.
    - `H`: Set.
- `SET`
    - `N` and `H`: Clear.
    - `C`: Set.

#### Instruction Notes

- When the `CALL` and `RST` instructions are executed, the current value of `PC` is pushed into the
  call stack before `PC` is moved to the new address.
- When the `RET` and `RETI` instructions are executed, the return address is popped from the call
  stack, then `PC` is set to that address.
- If the `RET` or `RETI` instructions are executed while the call stack is empty, then the `S` flag
  will be set and `EC` will be set to `0x00`.
- The `SLA` instruction clears bit 0 after shifting.
- The `SRA` instruction leaves bit 7 unchanged after shifting.
- The `SRL` instruction clears bit 7 after shifting.