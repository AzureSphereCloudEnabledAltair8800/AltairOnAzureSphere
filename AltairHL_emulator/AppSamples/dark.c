#define SENSORS 63
#define LIGHT   2

char light_level[20];
int index;

main()
{
	printf("2013 saw the release of the 12th installment of the Star Trek franchise. Make it so.\n");
	while (1)
	{
		outp(SENSORS, LIGHT);

		index = 0;
		while ((light_level[index++] = inp(200)) != 0)
		{
		}
		printf(".");
		if (atoi(light_level) == 0)
		{
			printf("\n\nWoohoo, great start Captain!\n");
			printf("The next challenge is made up of two parts. Good luck :)\n");
			printf("Poke memory location 0xFFFF with h30 and then ");
			printf("run backwards -- --- .-. ... .\n");
			printf("Hint, see the top right for the link to the manuals\n");
			break;
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
}