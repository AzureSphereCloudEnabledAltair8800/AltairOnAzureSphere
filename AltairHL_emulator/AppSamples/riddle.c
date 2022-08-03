int index;
char m[20];
int value;

main()
{
    if (peek(0xFFFE) != 0x44)
    {
        printf("Oh no, the escape room sequence key is incorrect. Did you miss a step?");
        exit()
    }

    printf("\n\nSIS SWIMS at NOON. Solve me and ");
    printf("you're one step closer to escaping the challenge :)\n\n");

    outp(64, 4);

    while (1)
    {
        outp(64, 5);
        outp(64, 2);
        index = 0;
        while (m[index++] = inp(200))
        {
        }
        value = atoi(m);
        printf(".");
        if (value < -1900)
        {
            break;
        }
        outp(29, 255);
        while (inp(29))
        {
        }
    }
    printf("\n\nNow for one last challenge. Run the supercomputer from the 1968 science fiction film.\n");
    printf("You'll need to switch back to the drive C:\n\n");
    poke(0xFFFE, 0x67);
}