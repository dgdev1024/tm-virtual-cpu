.org 0x3000         // Any origin before 0x80000000 is in ROM.
    main:
        ld d, count_low
        inc [d]
        jmp zc, [main]
        ld d, count_high
        inc [d]
        jmp zc, [main]
        stop

.org 0x80000000     // Any address from 0x80000000 is in RAM.
    count_high: .byte 1
    count_low:  .byte 1
