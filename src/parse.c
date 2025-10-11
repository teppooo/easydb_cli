#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"

void* xmalloc(size_t size)
{
	if (size == 0)
	{
		return NULL;
	}
	//tag memory for freeing on exit?
	return malloc(size);
}

int check_header(struct dbheader_s* headerPtr, unsigned int filesize)
{
	if (headerPtr->magic != HEADER_MAGIC ||
		headerPtr->version != DB_VERSION ||
		headerPtr->count > MAX_EMPLOYEE_COUNT ||
		headerPtr->filesize != filesize)
	{
		//ERRNO not set
		return FAIL;
	}
	return OK;
}

int create_db_header(struct dbheader_s **headerOut)
{
	if (headerOut == NULL)
	{
		return FAIL;
	}

	*headerOut = (struct dbheader_s *)xmalloc(sizeof(struct dbheader_s));
	if (*headerOut == NULL)
	{
		return FAIL;
	}

	(*headerOut)->magic = HEADER_MAGIC;
	(*headerOut)->version = 1;
	(*headerOut)->count = 0;
	(*headerOut)->filesize = 0;
	return OK;
}

int validate_db_header(int fd, struct dbheader_s **headerOut)
{
	if (headerOut == NULL)
	{
		//ERRNO not set
		return FAIL;
	}
	
	size_t headerSize = sizeof(struct dbheader_s);
	char headerBuf[headerSize];
	if (read(fd, headerBuf, headerSize) < headerSize)
	{
		return FAIL;
	}

	struct stat fileStat = {0};
	if (fstat(fd, &fileStat) < 0)
	{
		return FAIL;
	}

	struct dbheader_s* headerPtr = (struct dbheader_s*)headerBuf;
	if (check_header(headerPtr, (unsigned int)fileStat.st_size != OK))
	{
		//ERRNO not set
		return FAIL;
	}

	*headerOut = (struct dbheader_s *)xmalloc(headerSize);
	if (*headerOut == NULL)
	{
		return FAIL;
	}

	bcopy(headerPtr, *headerOut, headerSize);
	return OK;
}

int read_employees(int fd, struct dbheader_s *headerIn, struct employee_s **employeesOut)
{
	if (employeesOut == NULL)
	{
		//ERRNO not set
		return FAIL;
	}

	//so. this looks a bit funky.
	//basically we just want to check we're not fed some random junk data
	//outside of normal control flow.
	//magic is most likely to fail in junk data scenario,
	//it's good to verify count anyway before mallocing,
	//filesize shouldn't matter at this point since we are not(?) dealing
	//with the file itself anymore.
	if (check_header(headerIn, headerIn->filesize))
	{
		//ERRNO not set
		return FAIL;
	}
	
	size_t employeesSize = sizeof(struct employee_s) * headerIn->count;
	*employeesOut = (struct employee_s *)xmalloc(employeesSize);
	if (*employeesOut == NULL)
	{
		return FAIL;
	}
	
	if (read(fd, *employeesOut, employeesSize) < employeesSize)
	{
		free(*employeesOut);
		return FAIL;
	}
	return OK;
}

int output_file(int fd, struct dbheader_s *headerIn, struct employee_s *employees)
{
	if (check_header(headerIn, headerIn->filesize))
	{
		//ERRNO not set
		return FAIL;
	}
	
	if (write(fd, headerIn, sizeof(struct dbheader_s)) < sizeof(struct dbheader_s))
	{
		return FAIL;
	}
	if (write(fd, employees, sizeof(struct employee_s) * headerIn->count) <
			sizeof(struct employee_s) * headerIn->count)
	{
		return FAIL;
	}
	return OK;
}
