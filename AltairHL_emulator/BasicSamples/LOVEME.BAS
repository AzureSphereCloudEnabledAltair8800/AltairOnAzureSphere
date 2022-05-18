100 OUT 80,1 : REM Switch to font mode
200 TEXT$ = "Love, love me"
300 GOSUB 600
400 OUT 80,0 : REM Switch to bus mode
500 END
600 LENGTH% = LEN(TEXT$)
700 IF LENGTH% = 0 THEN RETURN
800 FOR INDEX% = 1 TO LENGTH%
900 OUT 85, ASC(MID$(TEXT$, INDEX%, 1)) : REM Write character to display
1000 OUT 29, 250 : WAIT 29, 1, 1  REM Delay for 250 milliseconds
1100 NEXT INDEX%
1200 RETURN

