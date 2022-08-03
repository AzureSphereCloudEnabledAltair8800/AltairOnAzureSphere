#define SENSORS 63
#define LIGHT   2

char light_level[20];
int level;
int index;
int dark;
int light;

main()
{
    dark  = 1;
    light = 1;
    light = -1;
    printf("\nWelcome to the Altair Escape Room Challenge.\n\nWe're glad you're are here.\n\n");
    printf("Ok, for the first challenge there are two clues:\n");
    printf("1) 2013 saw the release of the 12th installment of the Star Trek franchise. Make it so.\n");
    printf("2) The Boss is blind.\n\n");

    while (light || dark)
    {
        outp(SENSORS, LIGHT);

        index = 0;
        while ((light_level[index++] = inp(200)) != 0)
        {
        }
        printf(".");

        level = atoi(light_level);

        if (level == 0 && dark)
        {
            printf("\n\nNice work. Beam me up, Scotty!\n");
            dark = 0;

            if (light)
            {
                printf("Nearly there!\n");
            }

            continue;
        }

        if (level >= 100 && light)
        {
            printf("\n\nGreat work, clearly you're Springsteen fan. The Altair preferred Manfred Mann's cover :)\n");
            light = 0;

            if (dark)
            {
                printf("Nearly there!\n");
            }

            continue;
        }

        outp(29, 255);
        while (inp(29) == 1)
        {
        }
        outp(29, 255);
        while (inp(29) == 1)
        {
        }
    }
    poke(0xFFFE, 0xA6);

    printf("\n\nThe next challenge is made up of two parts. Good luck :)\n");
    printf("Poke memory location 0xFFFF with h30 and then ");
    printf("run backwards -- --- .-. ... .\n");
    printf("Hint, see the top right for the link to the manuals\n");
}