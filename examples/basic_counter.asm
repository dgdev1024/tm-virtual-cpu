.org 0x3000
    main:
        ld aw, 0x0000
        call n, [counter]
        stop

    counter:
        inc aw
        ret zs
        jpb n, [counter]