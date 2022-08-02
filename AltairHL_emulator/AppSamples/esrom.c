#define CODE       "SHAKE"
#define ROW7       97
#define ROW6       96
#define ROW5       95
#define ROW4       94
#define ROW3       93
#define ROW2       92
#define ROW1       91
#define ROW0       90
#define DRAW       102
#define CLEAR      101
#define PANEL_MODE 80
#define BITMAP     2
#define BUS        0

main(argc, argv) int argc;
char **argv;
{
	int level;
	char cmd;

	if (peek(0xFFFE) != 0xA6)
	{
		printf("Oh no, the escape room sequence key is incorrect. Did you miss a step?");
		exit();
	}

	if (peek(0xFFFF) != 0x30)
	{
		printf("Key at 0xFEFF is incorrect. try again");
		exit();
	}
	else 
	{
		printf("\nGreat working on poking that address\n");
		printf("Oh for the days of one address space OSs :)\n\n");
	}
	printf("------------------------------------\n");
	printf("See 8x8 LED panel for the next clue.\n");
	printf("------------------------------------\n\n");
	printf("Make it dark to quit ESROM.COM.\n");
	outp(PANEL_MODE, BITMAP);
	outp(CLEAR, 0);

	outp(ROW7, 168);
	outp(ROW6, 170);
	outp(ROW5, 176);
	outp(ROW4, 214);
	outp(ROW3, 128);
	outp(ROW2, 160);
	outp(ROW1, 192);
	outp(DRAW, 0);

	char light_level[20];
	int index;
	int i;

	while (1)
	{
		index = 0;
		outp(63, 2);
		while ((light_level[index++] = inp(200)))
		{
		}
		if (atoi(light_level) == 0)
		{
			break;
		}

		printf(".");
		outp(30, 1);
		while (inp(30) == 1)
		{
		}
	}
	outp(PANEL_MODE, BUS);
	poke(0xFFFE, 0x39);
}