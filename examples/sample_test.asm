// Sample Assembly Code for Testing TMM Lexer and Parser

// ROM Section
.org 0x3000
    start:
        // Data Transfer Instructions
        LD A, 0x1234       // Load immediate value into register A
        LD B, [0x12345678] // Load value from memory address into register B
        LDQ C, [0x0010]    // Load value from QRAM into register C
        LDH D, [0x20]      // Load value from hardware register into register D
        ST [0x12345678], A // Store value from register A into memory address
        STQ [0x0010], B    // Store value from register B into QRAM
        STH [0x20], C      // Store value from register C into hardware register
        MV D, A            // Move value from register A to register D
        PUSH A             // Push register A onto the stack
        POP B              // Pop value from the stack into register B

        // Control Transfer Instructions
        JMP NC, 0x2000     // Jump to address 0x2000 if no condition
        CALL CS, 0x3000    // Call subroutine at address 0x3000 if carry set
        RET ZS             // Return from subroutine if zero set
        RETI               // Return from interrupt
        JPS                // Jump to start address
        CALL NC, more      // Call subroutine at label 'more'.

    data:
        .byte 0x12, 0x34, 0x56, 0x78
        .word 0x1234, 0x5678
        .long 0x12345678

    more:
        // Arithmetic Instructions
        INC A              // Increment register A
        DEC B              // Decrement register B
        ADD A, 0x10        // Add immediate value to register A
        ADC B, A           // Add register A to register B with carry
        SUB C, 0x20        // Subtract immediate value from register C
        SBC D, B           // Subtract register B from register D with carry

        // Bitwise and Comparison Instructions
        AND A, 0x0F        // Bitwise AND register A with immediate value
        OR B, A            // Bitwise OR register B with register A
        XOR C, 0xFF        // Bitwise XOR register C with immediate value
        CMP D, A           // Compare register D with register A

        // Bit Shifting Instructions
        SLA A              // Arithmetic left shift register A
        SRA B              // Arithmetic right shift register B
        SRL C              // Logical right shift register C
        RL D               // Rotate left register D through carry
        RR A               // Rotate right register A through carry

        // Bit Checking Instructions
        BIT 0, A           // Check bit 0 of register A
        SET 1, B           // Set bit 1 of register B
        RES 2, C           // Clear bit 2 of register C
        SWAP D             // Swap upper and lower halves of register D
        STOP               // End Program