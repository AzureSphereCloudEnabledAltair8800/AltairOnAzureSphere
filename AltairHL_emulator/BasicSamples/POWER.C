main(argc, argv)
int argc;
char **argv;
{
  int level;
  char cmd;

  printf("Altair Power Manager\n");
  if (argc != 2){
    printf("\nExpected command line parameter:\n");
    printf("LED Panel brightness: (0..8). 0 disables panel.\n");
    printf("Power mode: Auto sleep. E = enable, D = disable, S = sleep\n");
    exit();
  }
  
  cmd = argv[1][0];

  level = atoi(argv[1]);

  if (strlen(argv[1]) == 1 && isdigit(cmd) && level >= 0 && level < 9) 
  {
     outp(65, level);
     printf("LED Brightness set to %d\n", level);
     exit();
  }

  if (cmd == 'E')
  {
     outp(66, 1);
     printf("Power management enabled.\n");  
     exit();
  }

  if (cmd == 'D')
  {
     outp(66, 0);
     printf("Power management disabled.\n");
     exit();
  }

  if (cmd == 'S')
  {
    outp(66, 2);
    printf("Power management sleep.\n");
    exit();
  }

   printf("Error: LED panel level (0..8), or e[nable], or d[isable].\n");
}