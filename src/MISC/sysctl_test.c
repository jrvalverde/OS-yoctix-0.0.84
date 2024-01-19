void doit (int a, int b)
  {
    int name[2];
    char str[200];
    int s = 200;

    name[0]=a;
    name[1]=b;

    memset (str, 65, 199);
    str[199]=0;
    sysctl(name, 2, str, &s, 0, 0);
    printf ("%i,%i: '%s'\n", a,b,str);
  }

main()
{
doit (1,1);
doit (1,10);
doit (1,2);
doit (1,27);
doit (1,4);
doit (6,1);
}
