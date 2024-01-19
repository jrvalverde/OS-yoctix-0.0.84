main (int argc, char *argv[])
  {
    int i;

    if (argc<2)
      {
	printf ("usage: %s nr\n"
		"where nr is one of the following:\n"
		"  1  read from NULL pointer\n"
		"  2  write to read-only memory\n", argv[0]);
	exit (0);
      }

    i = atoi (argv[1]);

    if (i==1)
      {
	char *p;
	char d;

	p = (char *) 0;
	printf ("About to read from NULL addr:\n");
	d = *p;
	printf ("Survived???\n");
	exit (1);
      }

    if (i==2)
      {
	char *p;

	p = (char *) 4096;
	printf ("About to write to read-only mem:\n");
	*p = 7;
	printf ("Survived???\n");
	exit (1);
      }

    printf ("Nothing to do.\n");
  }
