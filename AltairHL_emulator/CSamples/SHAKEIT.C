int index;
char prediction[30];
char light_level[20];

main(argc, argv) int argc;
char **argv;
{
	if (argc != 2)
	{
		printf("\nNice try, but I expected a key :)\n");
		exit();
	}

	if (strcmp(argv[1], "SHAKE"))
	{
		printf(
			"\nNice try, but wrong key\nRead those \"like a polaroid\" lyrics just a little bit closer :)\n");
		exit();
	}

	printf(
		"\nEureka, great work on the key! But you are not quite there yet. A hint is in those lyrics ;)\n\n");

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
			printf("The Azure Sphere device is running a TinyML (Tensorflow Lite) Movement Classification "
				   "model on one of the secure real-time cores.\nThe TinyML model was created with Edge "
				   "Impulse (www.edgeimpulse.com).\n");
			printf("\nNext steps:\n1. Change to the drive B:\n2. Copy riddle.c from Azure Sphere immutable "
				   "storage to the Altair filesystem: devget riddle.c\n3. Compile riddle.c: cc riddle\n4. "
				   "Link riddle: clink riddle\n5. Run riddle from the command line\n6. Good luck :)");
			break;
		}

		outp(30, 1);
		while (inp(30) == 1)
		{
		}
	}
	outp(64, 4);
}