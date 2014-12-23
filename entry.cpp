#include "acsm.h"

/*
*  Text Data Buffer
*/
unsigned char text[MAXLEN];
extern int nline;

int main(int argc, char **argv)
{
	int i, nocase = 0;
	FILE *fd;
	char filename[20];
	ACSM_STRUCT * acsm;

//	if (argc < 3)
//	{
//		fprintf(stderr, "Usage: acsmx filename pattern1 pattern2 ...  -nocase\n");
//		exit(0);
//	}

	acsm = acsmNew();

	strcpy_s(filename, "test.txt");
	fd = fopen(filename, "r");
	if (fd == NULL)
	{
		fprintf(stderr, "Open file error!\n");
		exit(1);
	}

	for (i = 1; i < argc; i++)
	if (strcmp(argv[i], "-nocase") == 0)
		nocase = 1;
	
	char a[] = { "test" };
	acsmAddPattern(acsm, (unsigned char *)a, strlen(a), nocase);

	/* Generate GtoTo Table and Fail Table */
	acsmCompile(acsm);

	/*Search Pattern*/
	while (fgets(( char*)text, MAXLEN, fd))
	{
		acsmSearch(acsm, text, strlen((char*)text), PrintMatch);
		nline++;
	}

	PrintSummary(acsm->acsmPatterns);

	acsmFree(acsm);

	printf("\n### AC Match Finished ###\n");
	system("pause");

	return (0);
}