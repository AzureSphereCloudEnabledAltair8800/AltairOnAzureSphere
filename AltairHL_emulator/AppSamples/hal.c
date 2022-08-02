#include <stdio.h>

FILE *fp_input;
char *key;
char buf[128];

main(argc, argv) int argc;
char **argv;
{
    char c;

	if (peek(0xFFFE) != 0x67)
	{
		printf("Oh no, the escape room sequence key is incorrect. Did you miss a step?");
		exit();
	}

	printf("\n2001: Space Odyssey is a 1968 epic science fiction film produced and directed by Stanley ");
	printf("Kubrick.\n");
	printf("The movie featured HAL, a supercomputer gone rouge.\n\n");

	while (get_key() == -1)
	{
	}

    printf("Congratulations, you have completed the Altair Escape Room Challenge!\n\n");
    printf("Take a screenshot of this code: "); 
    /* get the first 8 chars of the Azure Sphere Device ID */
    outp(72, 0);
    while((c = inp(200)) != 0 )
    {
        printf("%c",c);
    }
    printf("\n\nAnd locate one of Altair Escape Room Challenge organizers for information on prizes.");

}

int get_key()
{
	char key_validate[128];
	int c;
	int len;

	printf("What came after HAL? ");

	key = fgets(buf, 128, fp_input);

	len = strlen(key);
	len--;
	key[len] = 0x00;

	for (c = 0; c < len; c++)
	{
		key_validate[c] = toupper(key[c]);
	}
	key_validate[len] = 0x00;

	if (strcmp(key_validate, "IBM"))
	{
		printf("\nNice try, but '%s' did not come after HAL\n", key);
		return -1;
	}
	else
	{
		printf("\nWoohoo, congratulations, you did it, you escaped!\n\n");
	}

	return 0;
}