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

void print_usage(char **av)
{
	printf("Usage: %s -n -f <database file>\n", av[0]);
	printf("\t -n - create new database file\n");
	printf("\t -f - (required) path to database file\n");
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

		if (output_file(dbfd, headerPtr, NULL)) //no employees to output yet
		{
			return -1;
		}
		printf("Database created.\n");
		close(dbfd);
		return 0;
	}

	struct dbheader_t* headerPtr = NULL;
	dbfd = open(filepath, O_RDWR);
	if (dbfd == -1)
	{
		perror("open db");
		return -1;
	}

	if (validate_db_header(dbfd, &headerPtr) == STATUS_SUCCESS)
	{
		printf("main: magic:%u version:%u count:%u filesize:%u\n",
			headerPtr->magic,
			headerPtr->version,
			headerPtr->count,
			headerPtr->filesize);
		printf("good job :)\n");
	}
	else
	{
		printf("oops :(\n");
		return -1;
	}

	if (addstring == NULL && list == false)
	{
		//guess we can exit early
		free(headerPtr);
		return 0;
	}

	struct employee_t* employeesPtr = NULL;
	if (headerPtr->count > 0 && read_employees(dbfd, headerPtr, &employeesPtr) == STATUS_ERROR)
	{
		return -1;
	}

	if (addstring != NULL)
	{
		headerPtr->count++;
		employeesPtr = realloc(employeesPtr, sizeof(struct employee_t) * headerPtr->count);
		if (employeesPtr == NULL)
		{
			perror("realloc");
			free(headerPtr);
			return -1;
		}
		if (add_employee(headerPtr, employeesPtr, addstring) == STATUS_ERROR)
		{
			free(headerPtr);
			free(employeesPtr);
			return -1;
		}
		printf("Added employee %s, new count %u\n",
				employeesPtr[headerPtr->count - 1].name,
				headerPtr->count);
	}

	if (output_file(dbfd, headerPtr, employeesPtr))
	{
		free(headerPtr);
		free(employeesPtr);
		return -1;
	}
	free(headerPtr);
	free(employeesPtr);
	return 0;
}
