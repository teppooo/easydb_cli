#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "file.h"
#include "parse.h"
#include "common.h"

void xfree(void *ptr)
{
	if (ptr != NULL)
	{
		free(ptr);
	}
}

void print_usage(char **av)
{
	printf("Usage: %s -n -f <database file>\n", av[0]);
	printf("\t -n -- create new database file\n");
	printf("\t -f -- (required) path to database file\n");
	printf("\t -a -- add employee via CSV list of (name,address,hours)\n");
}

int main(int ac, char** av)
{
	char *filepath = NULL;
	char *addstring = NULL;
	bool newfile = false;
	bool list = false;
	int dbfd = -1;
	int c = 0;

	while ((c = getopt(ac, av, "nf:a:")) != -1)
	{
		switch (c) {
			case 'n':
				newfile = true;
				break;
			case 'f':
				filepath = optarg;
				break;
			case 'a':
				addstring = optarg;
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

	printf("Newfile: %s\n", newfile ? "true" : "false");
	printf("Filepath: %s\n", filepath);

	struct dbheader_t* headerPtr = NULL;
	if (newfile)
	{
		dbfd = create_db_file(filepath);
		if (dbfd == -1)
		{
			return -1;
		}
		if (create_db_header(&headerPtr) == STATUS_ERROR)
		{
			return -1;
		}
		printf("Database created.\n");
	}
	else
	{
		dbfd = open(filepath, O_RDWR);
		if (dbfd == -1)
		{
			perror("open db");
			return -1;
		}
		if (validate_db_header(dbfd, &headerPtr) == STATUS_ERROR)
		{
			return -1;
		}
	}

	printf("main: magic:%u version:%u count:%u filesize:%u\n",
		headerPtr->magic,
		headerPtr->version,
		headerPtr->count,
		headerPtr->filesize);
	printf("good job so far :)\n");

	struct employee_t* employeesPtr = NULL;
	if (headerPtr->count > 0 && read_employees(dbfd, headerPtr, &employeesPtr) == STATUS_ERROR)
	{
		xfree(headerPtr);
		xfree(employeesPtr);
		return -1;
	}

	if (addstring != NULL)
	{
		if (add_employee(headerPtr, &employeesPtr, addstring) == STATUS_ERROR)
		{
			xfree(headerPtr);
			xfree(employeesPtr);
			return -1;
		}
		printf("Added employee %s, new count %u\n",
				employeesPtr[headerPtr->count - 1].name,
				headerPtr->count);
	}

	if (output_file(dbfd, headerPtr, employeesPtr))
	{
		xfree(headerPtr);
		xfree(employeesPtr);
		return -1;
	}
	xfree(headerPtr);
	xfree(employeesPtr);
	return 0;
}
