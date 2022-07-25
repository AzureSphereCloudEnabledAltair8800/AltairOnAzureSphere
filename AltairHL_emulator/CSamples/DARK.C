#define SENSORS 63
#define LIGHT   2

char light_level[20];
int index;

main()
{
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
			printf("\nPoke memory location hFEFF with h30 and then ");
			printf("run backwards -- --- .-. ... .\n");
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
}