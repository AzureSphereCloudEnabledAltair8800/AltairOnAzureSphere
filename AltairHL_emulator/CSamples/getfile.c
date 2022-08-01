#include <stdio.h>
#define STG_FILENAME 68
#define STG_EOF      68
#define STG_GET_BYTE 202

#define WG_EPNAME   110
#define WG_GET_URL  111
#define WWG_SET_URL 112
#define WG_SELECTED 113
#define WG_FILENAME 114
#define WG_GET_BYTE 201
#define WG_STATUS   33

#define WG_EOF       0
#define WG_WAITING   1
#define WG_DATAREADY 2
#define WG_FAILED    3

#define GET_STRING 200

FILE *fp_input;
FILE *fp_output;
char *endpoint;
char *filename;
char buf[128];
int selected_ep;

main(argc, argv) char **argv;
int argc;
{
	int source;
	int wg_result;

	fp_input = fopen("stdin", "r");

	printf("\nFile copy utility.\n");
	printf("Get file from Azure Sphere storage or the web over HTTP(s).\n");

	if (argc == 2)
	{
		while (get_endpoint() == -1)
		{
		}

		set_ep_url(endpoint);
		printf("\nEndpoint URL set to: %s\n", endpoint);
		exit();
	}

	if (argc == 1)
	{
		while (set_selected() == -1)
		{
		}

		while (get_filename() == -1)

		printf("\nTransferring file '%s' from endpoint '%d'\n\n", filename, selected_ep);
	}

	if ((fp_output = fopen(filename, "w")) == NULL)
	{
		printf("Failed to open %s\n", filename);
		exit();
	}

	switch (selected_ep)
	{
		case 0:
			set_stg_fname(filename, strlen(filename), STG_FILENAME);
			immutable_copy();
			break;
		case 1:
			set_fname_web(filename, strlen(filename), 0);
			wg_result = web_copy();
			break;
		case 2:
			set_fname_web(filename, strlen(filename), 1);
			wg_result = web_copy();
			break;
		default:
			break;
	}

	fclose(fp_output);
	if (wg_result == -1)
	{
		printf("\n\nWeb copy failed for file '%s'. Check filename and network connection\n", filename);
		unlink(filename);
	}
}

void set_ep_url(endpoint) char *endpoint;
{
	int len;
	int c;
	len = strlen(endpoint);
	for (c = 0; c < len; c++)
	{
		outp(WG_EPNAME, endpoint[c]);
	}

	outp(WG_EPNAME, 0);
}

/* get personal endpoint url */
void get_ep_url(endpoint) int endpoint;
{
	printf("2) ");
	char c;
	outp(WG_GET_URL, 0);
	while ((c = inp(GET_STRING)) != 0)
	{
		printf("%c", c);
	}
	printf("%c", '\n');
}

/* Get selected endpoint URL */
void get_selected_ep()
{
	printf("Selected endpoint: ");
	char c;
	outp(WG_SELECTED, 0);
	while ((c = inp(GET_STRING)) != 0)
	{
		printf("%c", c);
	}
	printf("%c", '\n');
}

/* Sets the filename to be copied with i8080 port 68 */
int set_stg_fname(filename, len, port)
char *filename;
int len;
int port;
{
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

/* set the selected endpoint url */
int set_selected()
{
	char *endpoint;

	printf("Select endpoint:\n");
	printf("0) Azure Sphere immutable storage\n");
	printf("1) https://raw.githubusercontent.com/AzureSphereCloudEnabledAltair8800/RetroGames/main\n");
	get_ep_url(1);

	printf("\nSelect endpoint number: ");
	endpoint = fgets(buf, 128, fp_input);

	if (strlen(endpoint) > 2)
	{
		return -1;
	}

	if (endpoint[0] >= '0' && endpoint[0] <= '2')
	{
		selected_ep = atoi(endpoint);
		return 0;
	}
	return -1;
}

/* Sets the filename to be copied with i8080 port 68 */
int set_fname_web(filename, len, endpoint)
char *filename;
int len;
int endpoint;
{
	/* Set web endpoint */
	outp(WWG_SET_URL, endpoint);

	int c;
	for (c = 0; c < len; c++)
	{
		outp(WG_FILENAME, filename[c]);
	}
	outp(WG_FILENAME, 0);
}

/* copies file byte stream */
int web_copy()
{
	char status;

	while ((status = inp(WG_STATUS)) != WG_EOF)
	{
		if (status == WG_FAILED)
		{
			return -1;
		}

		if (status == WG_DATAREADY)
		{
			fputc(inp(WG_GET_BYTE), fp_output);
			printf(".");
		}
		else
		{
			printf("*");
		}
	}

	return 0;
}

int get_filename()
{
	int len;

	printf("Enter filename to transfer: ");
	filename = fgets(buf, 128, fp_input);

	len = strlen(filename);

	if (len < 1 || len > 13)
	{
		printf("Filename must be 1 to 12 characters. Try again.\n");
		return -1;
	}

	filename[len - 1] = 0x00;

	return 1;
}

int get_endpoint()
{
	char url[5];
	int c;
    int len;

	printf("\nEnter endpoint url: ");

	endpoint = fgets(buf, 128, fp_input);

    len = strlen(endpoint);
    len--;
    endpoint[len] = 0x00;

	if (len < 8)
	{
		printf("Invalid endpoint url.\n");
		return -1;
	}

	for (c = 0; c < 4; c++)
	{
		url[c] = toupper(endpoint[c]);
	}
	url[4] = 0x00;

	if (strcmp(url, "HTTP"))
	{
		printf("Invalid endpoint url. Must start with 'http'.\n");
		return -1;
	}

	return 0;
}