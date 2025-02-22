.org 0x3000
    main:
        ld d, count_low
        inc [d]
        jmp zc, [main]
        ld d, count_high
        inc [d]
        jmp zc, [main]
        stop

.org 0x80000000              // Any address starting from 0x80000000 is in RAM.
    count_high: byte
    count_low:  byte
