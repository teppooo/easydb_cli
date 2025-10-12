#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>

#include "file.h"
#include "parse.h"
#include "common.h"

void print_usage(char **av)
{
	printf("Usage: %s -n -f <database file>\n", av[0]);
	printf("\t -n - create new database file\n");
	printf("\t -f - (required) path to database file\n");
}

int main(int ac, char** av)
{
	char *filepath = NULL;
	bool newfile = false;
	int dbfd = -1;
	int c = 0;

	while ((c = getopt(ac, av, "nf:")) != -1)
	{
		switch (c) {
			case 'n':
				newfile = true;
				break;
			case 'f':
				filepath = optarg;
				break;
			case '?':
				printf("Unknown option -%c\n", c);
				break;
			default:
				return -1;
		}
	}
	
	if (filepath == NULL)
	{
		printf("Filepath is required argument\n");
		print_usage(av);
		return -1;
	}

	if (newfile)
	{
		dbfd = create_db_file(filepath);
		if (dbfd == -1)
		{
			return -1;
		}
		
		struct dbheader_t* headerPtr = NULL;
		if (create_db_header(&headerPtr) == STATUS_ERROR)
		{
			return -1;
		}
		
/*		if (headerPtr->count > 0)
		{
			if (read_employees(dbfd, headerPtr, &employeesPtr))
			{
				return -1;
			}
		}
*/
		if (output_file(dbfd, headerPtr, NULL)) //no employees to output yet
		{
			return -1;
		}
	}
	printf("Newfile: %s\n", newfile ? "true" : "false");
	printf("Filepath: %s\n", filepath);

	return 0;
}
