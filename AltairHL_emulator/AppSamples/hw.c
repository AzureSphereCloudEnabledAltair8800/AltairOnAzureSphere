/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

/* C application to demonstrate use of Intel 8080 IO Ports */

main()
{
    unsigned c, l;
    char buffer[50];

    outp(64, 4); /* stop accelerometer if it's running */
    outp(60, 0); /* Turn of red LED if it's on */

    printf("\nHello from the Altair 8800 emulator\n\n");

    for (c = 0; c < 65535; c++)
    {
        outp(60, 1);  /* Turn on the red LED */

        printf("%c[91;22;24mReading: %u\n", 27, c);
        printf("--------------------------------------------%c[0m\n", 27);
        printf("System tick count: \t%s\n", get_port_data(41, 0,  buffer, 50));
        printf("UTC date and time: \t%s\n", get_port_data(42, 0,  buffer, 50));
        printf("Local date and time: \t%s\n", get_port_data(43, 0, buffer, 50));
        printf("Altair version: \t%s\n", get_port_data(70, 0, buffer, 50));
        printf("Azure Sphere OS: \t%s\n\n", get_port_data(71, 0, buffer, 50));

        printf("Onboard temperature: \t%s\n", get_port_data(63, 0, buffer, 50));
        printf("Onboard pressure: \t%s\n", get_port_data(63, 1, buffer, 50));
        printf("Onboard light sensor: \t%s\n\n", get_port_data(63, 2, buffer, 50));

        outp(64, 5); /* read the accelerometer */
        printf("Accelerometer X: \t%s\n", get_port_data(64, 0, buffer, 50));
        printf("Accelerometer Y: \t%s\n", get_port_data(64, 1, buffer, 50));
        printf("Accelerometer Z: \t%s\n\n", get_port_data(64, 2, buffer, 50));

/*      printf("Prediction: \t%s\n\n", get_port_data(64, 8, buffer, 50));
*/
        outp(60, 0); /* turn off the red LED */

        sleep(2); /* Sleep for 2 seconds */
    }
}

/* Sleep for n seconds */
sleep(seconds)
char seconds;
{
    outp(30, seconds); /* Enable sleep for N seconds */
    while (inp(30)); /* Wait for sleep to expire */
}

/* Get data from Intel 8080 IO port */
char *get_port_data(port_num, data, buffer, buffer_len)
int port_num;
int data;
char *buffer;
int buffer_len;
{
    char ch;
    int index;

    index = 0;

    /* Select data to be read */
    outp(port_num, data);

    while ((ch = inp(200)) && index < buffer_len) {
        buffer[index++] = ch;
    }
    buffer[index] = 0x00;

    return buffer;
}