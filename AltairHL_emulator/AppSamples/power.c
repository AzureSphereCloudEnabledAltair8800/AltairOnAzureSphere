#define PORT_LED     65
#define PORT_POWER   66
#define PORT_DISABLE 0
#define PORT_ENABLE  1
#define PORT_SLEEP   2

main(argc, argv) int argc;
char **argv;
{
    int level;
    char cmd;

    printf("Altair Power Manager\n");
    if (argc != 2)
    {
        printf("\nExpected command line parameter:\n");
        printf("LED Panel brightness: (0..8). 0 disables panel.\n");
        printf("Power mode: Auto sleep. E = enable, D = disable, S = sleep\n");
        exit();
    }

    cmd = argv[1][0];

    level = atoi(argv[1]);

    if (strlen(argv[1]) == 1 && isdigit(cmd) && level >= 0 && level < 9)
    {
        outp(PORT_LED, level);
        printf("LED Brightness set to %d\n", level);
        exit();
    }

    if (cmd == 'E')
    {
        outp(PORT_POWER, PORT_ENABLE);
        printf("Power management enabled.\n");
        exit();
    }

    if (cmd == 'D')
    {
        outp(PORT_POWER, PORT_DISABLE);
        printf("Power management disabled.\n");
        exit();
    }

    if (cmd == 'S')
    {
        outp(PORT_POWER, PORT_SLEEP);
        printf("Power management sleep.\n");
        exit();
    }

    printf("Error: LED panel level (0..8), or e[nable], or d[isable].\n");
}