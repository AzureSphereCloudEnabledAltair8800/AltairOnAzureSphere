        ORG     0100H           ; CP/M base of TPA (transient program area)
        MVI     A, 0            ; Set retro click display to bus mode 
        OUT     80              ; Call port to set display mode 
        MVI     C,09H           ; Print string function
        LXI     D,MESSAGE       ; Point
        CALL    0005H           ; Call bdos
        MVI     C, 09H          ; Print string function
        LXI     D, RESET        ; Point to vt200 reset sequence
        CALL    0005H           ; Call bdos
        RET                     ; To cp/m
MESSAGE:DB      027,'[2J','$'
RESET:  DB      027,'[0m','$' 
        END