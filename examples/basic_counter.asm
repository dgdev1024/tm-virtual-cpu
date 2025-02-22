.org 0x3000
    main:
        ld aw, 0x0000
        call nc, [counter]
        stop

    counter:
        inc aw
        ret zs
        jpb nc, [counter]