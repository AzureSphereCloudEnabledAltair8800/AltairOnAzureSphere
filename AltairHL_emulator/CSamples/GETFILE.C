#include <stdio.h>
#define STG_FILENAME 68
#define STG_EOF      68
#define STG_GET_BYTE 202
#define WEB_FILENAME 33
#define WEB_EOF      33
#define WEB_GET_BYTE 201

FILE *fp_output;

main(argc, argv) char **argv;
int argc;
{
	int source;
	char *filename;

	printf("\nFile copy utility.\n");
	printf("Copy files from Azure Sphere immutable storage and the web over HTTP(s).\n\n");

	if (!validate_input(argc, argv))
	{
		exit();
	}

	filename = argv[2];
	source   = atoi(argv[1]);

	if ((fp_output = fopen(filename, "w")) == NULL)
	{
		printf("Failed to open %s\n", filename);
		exit();
	}

	switch (source)
	{
		case 0:
			set_filename_immutable(filename, strlen(filename), STG_FILENAME);
			immutable_copy();
			break;
		case 1:
			set_filename_immutable(filename, strlen(filename), WEB_FILENAME);
			web_copy();
			break;
		default:
			break;
	}

	fclose(fp_output);
}

/* Sets the filename to be copied with i8080 port 68 */
int set_filename_immutable(filename, len, port)
char *filename;
int len;
int port;
{
	printf("filename on port: %d\n", port);
	int c;
	for (c = 0; c < len; c++)
	{
		outp(port, filename[c]);
	}
	outp(port, 0);
	printf("filename set\n");
}

/* copies file byte stream from i8080 port 202 */
void immutable_copy()
{
	char ch;

	/* While not end of file read in next byte */
	while ((ch = inp(STG_EOF)) == 0)
	{
		fputc(inp(STG_GET_BYTE), fp_output);
		printf(".");
	}
}

/* copies file byte stream from i8080 port 33 */
void web_copy()
{
	/* Wait for the HTTP GET request to complete and file is loaded */
	/* End of file flag goes low when file is ready to copy */
	while (inp(WEB_EOF) == 1)
	{
	}

	/* While not end of file read in next byte */
	while (inp(WEB_EOF) == 0)
	{
		fputc(inp(WEB_GET_BYTE), fp_output);
		printf(".");
	}
}

int validate_input(argc, argv)
char **argv;
int argc;
{
	char *filename;
	int source;

	if (argc != 3)
	{
		printf("Usage: getfile source filename\n");
		printf("Source is numeric. 0 = immutable storage, 1 = web\n");
		return 0;
	}

	filename = argv[2];

	/* Filename length max 8 + . + 3 char extension FILENAME.TXT */
	if (strlen(filename) > 12)
	{
		printf("Invalid filename. Too long!\n");
		return 0;
	}

	if (strlen(argv[1]) == 1 && isdigit(argv[1][0]))
	{
		printf("Source error: 0 = immutable storage, 1 = web\n");
		return 0;
	}

	source = atoi(argv[1]);

	if (source < 0 || source > 1)
	{
		printf("Invalid source number: 0 = immutable storage, 1 = web\n");
		return 0;
	}

	return 1;
}