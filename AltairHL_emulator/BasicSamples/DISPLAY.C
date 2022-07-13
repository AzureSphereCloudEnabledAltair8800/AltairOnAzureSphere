main(argc, argv)
int argc;
char **argv;
{
  int level;
  printf("Set Mikroe Retro Click LED brightness level\n");
  if (argc != 2){
    printf("Expected numeric brightness between 0 and 8. 0 turns off display, 8 is max brightness");
  }
  else
  {
    level = atoi(argv[1]);
    if (strlen(argv[1]) == 1 && isdigit(*argv[1]) && level >= 0 && level < 9) 
    {
       outp(65, level);
       printf("LED Brightness set to %d\n", level);
    }
    else
    {
      printf("Error: Expected numeric argument between 0 and 8 inclusive. \n");
    }
  }
}