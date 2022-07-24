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

	/*printf("\n\nLike a polaroid picture!\n");
	  if (argc != 2){
		printf("\nExpected key;)\n");
		exit();
	  }
	*/
	if (peek(0xFEFF) != 0x30)
	{
		printf("Code at 0xFEFF is incorrect. try again");
		exit();
	}

	/*  if (strcmp(argv[1], CODE) != 0)
	  {
		printf("Well that's awkward, badong, try again :)\n");
		exit();
	  }
	*/
	printf("See 8x8 LED panel for the next clue.\n");
	printf("Make it dark to quit esrom.\n");
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
}