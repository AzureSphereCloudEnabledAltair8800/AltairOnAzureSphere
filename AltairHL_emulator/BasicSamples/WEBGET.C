#include <stdio.h>
#define PORT_FILENAME 33
#define PORT_EOF 33
#define PORT_GET_BYTE 201

FILE *fp_output;

main(argc, argv) char **argv;
{
    if (argc != 2) {
        printf("Usage: webget filename\n");
        exit();
    }

    int filename_len;
    filename_len = strlen(argv[1]);

    /* Filename length max 8 + . + 3 char extension FILENAME.TXT */
    if (filename_len > 12) {
        printf("Invalid filename. Too long!\n");
        exit();
    }

    if ((fp_output = fopen(argv[1], "w")) == NULL) {
        printf("Failed to open %s\n", argv[1]);
        exit();
    }

    /* Sets the filename to be copied with i8080 port 33 */
    set_filename(argv[1], filename_len);

    /* copies file byte stream from i8080 port 33 */
    copy_file();

    fclose(fp_output);
}

/* Sets the filename to be copied with i8080 port 33 */
int set_filename(filename, len)
char *filename; int len;
{
    int c;
    for (c = 0; c < len; c++) {
        outp(PORT_FILENAME, filename[c]);
    }
    outp(PORT_FILENAME, 0);
}

/* copies file byte stream from i8080 port 33 */
void copy_file()
{
    /* Wait for the HTTP GET request to complete and file is loaded */
    /* End of file flag goes low when file is ready to copy */
    while(inp(PORT_EOF) == 1){}

    /* While not end of file read in next byte */
    while (inp(PORT_EOF) == 0) {
       fputc(inp(PORT_GET_BYTE), fp_output);
    }
}