#include <stdio.h>

FILE *fp_output;

main(argc, argv) char **argv;
{
    if (argc != 2) {
        printf("Usage: devget filename\n");
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

    /* Sets the filename to be copied with i8080 port 67 */
    set_filename(argv[1], filename_len);

    /* copies file byte stream from i8080 port 202 */
    copy_file();

    fclose(fp_output);
}

/* Sets the filename to be copied with i8080 port 67 */
int set_filename(filename, len)
char *filename; int len;
{
    int c;
    for (c = 0; c < len; c++) {
        outp(67, filename[c]);
    }
    outp(67, 0);
}

/* copies file byte stream from i8080 port 202 */
void copy_file()
{
    char ch;

    /* While not end of file read in next byte */
    while ((ch = inp(67)) == 0) {
       fputc(inp(202), fp_output);
    }
}