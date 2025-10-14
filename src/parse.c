#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>

#include "parse.h"
#include "common.h"

void* xmalloc(size_t size)
{
	if (size == 0)
	{
		return NULL;
	}
	return malloc(size);
}

int check_header(struct dbheader_t* headerPtr, unsigned int filesize)
{
	if (headerPtr->magic != HEADER_MAGIC ||
		headerPtr->version != DB_VERSION ||
		headerPtr->filesize != filesize)
	{
		//ERRNO not set
		return STATUS_ERROR;
	}
	return STATUS_SUCCESS;
}

int create_db_header(struct dbheader_t **headerOut)
{
	if (headerOut == NULL)
	{
		//ERRNO not set
		return STATUS_ERROR;
	}

	*headerOut = (struct dbheader_t *)xmalloc(sizeof(struct dbheader_t));
	if (*headerOut == NULL)
	{
		return STATUS_ERROR;
	}

	(*headerOut)->magic = HEADER_MAGIC;
	(*headerOut)->version = 1;
	(*headerOut)->count = 0;
	(*headerOut)->filesize = sizeof(struct dbheader_t);
	return STATUS_SUCCESS;
}

int validate_db_header(int fd, struct dbheader_t **headerOut)
{
	if (fd < 0)
	{
		//ERRNO not set
		return STATUS_ERROR;
	}

	if (headerOut == NULL)
	{
		//ERRNO not set
		return STATUS_ERROR;
	}
	
	const ssize_t headerSize = sizeof(struct dbheader_t);
	char headerBuf[headerSize];
	if (read(fd, headerBuf, headerSize) < headerSize)
	{
		return STATUS_ERROR;
	}

	struct stat fileStat = {0};
	if (fstat(fd, &fileStat) < 0)
	{
		return STATUS_ERROR;
	}

	struct dbheader_t* headerPtr = (struct dbheader_t*)headerBuf;
	if (check_header(headerPtr, (ssize_t)fileStat.st_size) != STATUS_SUCCESS)
	{
		//ERRNO not set
		return STATUS_ERROR;
	}

	*headerOut = (struct dbheader_t *)xmalloc(headerSize);
	if (*headerOut == NULL)
	{
		return STATUS_ERROR;
	}

	bcopy(headerPtr, *headerOut, headerSize);
	return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *headerIn, struct employee_t **employeesOut)
{
	if (fd < 0)
	{
		//ERRNO not set
		return STATUS_ERROR;
	}

	if (employeesOut == NULL)
	{
		//ERRNO not set
		return STATUS_ERROR;
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
		return STATUS_ERROR;
	}
	
	//I'm assuming we will validate the employees also at some point some day
	ssize_t employeesSize = sizeof(struct employee_t) * headerIn->count;
	*employeesOut = (struct employee_t *)xmalloc(employeesSize);
	if (*employeesOut == NULL)
	{
		return STATUS_ERROR;
	}
	
	if (read(fd, *employeesOut, employeesSize) < employeesSize)
	{
		free(*employeesOut);
		return STATUS_ERROR;
	}
	return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *headerIn, struct employee_t *employees)
{
	if (fd < 0)
	{
		//ERRNO not set
		return STATUS_ERROR;
	}

	if (headerIn != NULL && check_header(headerIn, headerIn->filesize))
	{
		//ERRNO not set
		return STATUS_ERROR;
	}
	headerIn->filesize = sizeof(struct dbheader_t) + sizeof(struct employee_t) * headerIn->count;
	
	if (write(fd, headerIn, sizeof(struct dbheader_t)) < (ssize_t)sizeof(struct dbheader_t))
	{
		return STATUS_ERROR;
	}

	if (employees == NULL)
	{
		if (headerIn->count == 0)
		{
			//no employees to write
			return STATUS_SUCCESS;
		}
		return STATUS_ERROR;
	}
	if (write(fd, employees, sizeof(struct employee_t) * headerIn->count) <
			(ssize_t)sizeof(struct employee_t) * headerIn->count)
	{
		return STATUS_ERROR;
	}
	return STATUS_SUCCESS;
}
