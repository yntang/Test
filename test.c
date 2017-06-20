main(argc, argv)
int argc;
char *argv[];
{
  int i;

  printf("%d:\n",argc);
  for(i=0;i<argc;i++) {
	printf("%d: %s\n",i,argv[i]);
  }
}
  
