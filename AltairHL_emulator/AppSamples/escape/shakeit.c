#include <stdio.h>

FILE *fp_input;
char *key;
char buf[128];
char light_level[20];
char prediction[30];
int index;

main(argc, argv) int argc;
char **argv;
{

    if (peek(0xFFFE) != 0x39)
    {
        printf("Oh no, the escape room sequence key is incorrect. Did you miss a step?");
        exit();
    }

    printf("\nRead those \"like a polaroid\" lyrics just a little bit closer :)\n");
    while (get_key() == -1)
    {
    }

    outp(64, 3); /* start the accelerometer */
    while (1)
    {
        index = 0;

        outp(64, 8); /* load the prediction */
        while ((prediction[index++] = inp(200)))
        {
        }

        /* printf("The current movement prediction is: %s\n", prediction); */
        printf(".");
        if (strcmp(prediction, "normal"))
        {
            printf("\n\nCongratulations, you did it, you've almost escaped :)\n");
            printf("The Azure Sphere device is running a TinyML (Tensorflow Lite) Movement Classification ");
            printf("model on one of the secure real-time cores.\nThe TinyML model was created with Edge ");
            printf("Impulse (www.edgeimpulse.com).\n");
            printf("\nNext steps:\n");
            printf("1. Change to the drive B:\n");
            printf("2. Copy riddle.c from Azure Sphere immutable storage to the Altair filesystem. Follow ");
            printf("these steps:\n\n");
            printf("\tRun 'gf' (short for getfile)\n");
            printf("\tSelect option 0\n");
            printf("\tNext, type the filename 'riddle.c'\n");
            printf("\tPress enter to start the transfer\n\n");
            printf("3. Compile riddle.c: cc riddle\n");
            printf("4. Link riddle: clink riddle\n");
            printf("5. Run riddle from the command line\n");
            printf("6. Good luck :)");
            break;
        }

        /* sleep for 1 second */
        outp(30, 1);
        while (inp(30) == 1)
        {
        }
    }
    /* turn off the accelerometer */
    outp(64, 4);
    poke(0xFFFE, 0x44);
}

int get_key()
{
    char key_validate[128];
    int c;
    int len;

    printf("Enter the key: ");

    key = fgets(buf, 128, fp_input);

    len = strlen(key);
    len--;
    key[len] = 0x00;

    for (c = 0; c < len; c++)
    {
        key_validate[c] = toupper(key[c]);
    }
    key_validate[len] = 0x00;

    if (strcmp(key_validate, "SHAKE"))
    {
        printf("\nNice try, but wrong key\nRead those \"like a polaroid\" lyrics just a little bit closer :)\n");
        return -1;
    }
    else
    {
        printf("\nEureka, great work on the key! But you are not quite there yet. A hint is in those lyrics "
               ";)\n\n");
    }

    return 0;
}