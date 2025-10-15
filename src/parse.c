#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stddef.h>

#include "parse.h"
#include "common.h"

void* xmalloc(size_t size)
{
	if (size == 0)
	{
		return NULL;
	}
	return calloc(1, size);
}

int check_header(struct dbheader_t* headerPtr, unsigned int filesize)
{
	if (headerPtr->magic != HEADER_MAGIC ||
		headerPtr->version != DB_VERSION ||
		headerPtr->filesize != filesize)
	{
		return STATUS_ERROR;
	}
	return STATUS_SUCCESS;
}

int create_db_header(struct dbheader_t **headerOut)
{
	if (headerOut == NULL)
	{
		printf("Can't create header\n");
		return STATUS_ERROR;
	}

	*headerOut = (struct dbheader_t *)xmalloc(sizeof(struct dbheader_t));
	if (*headerOut == NULL)
	{
		perror("create header");
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
	if (fd < 0 || headerOut == NULL)
	{
		printf("Error while validating header\n");
		return STATUS_ERROR;
	}
	
	struct stat fileStat = {0};
	if (fstat(fd, &fileStat) < 0)
	{
		perror("read filestat");
		return STATUS_ERROR;
	}

	const ssize_t headerSize = sizeof(struct dbheader_t);
	struct dbheader_t* headerPtr = (struct dbheader_t *)xmalloc(headerSize);
	if (read(fd, headerPtr, headerSize) < headerSize)
	{
		perror("read header");
		return STATUS_ERROR;
	}

	headerPtr->magic = ntohl(headerPtr->magic);
	headerPtr->version = ntohs(headerPtr->version);
	headerPtr->count = ntohs(headerPtr->count);
	headerPtr->filesize = ntohl(headerPtr->filesize);

	if (check_header(headerPtr, (ssize_t)fileStat.st_size) != STATUS_SUCCESS)
	{
		printf("Bad header\n");
		free(headerPtr);
		return STATUS_ERROR;
	}
	*headerOut = headerPtr;
	return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *headerIn, struct employee_t **employeesOut)
{
	if (fd < 0 || employeesOut == NULL)
	{
		printf("Error while reading employees\n");
		return STATUS_ERROR;
	}

	if (headerIn->magic != HEADER_MAGIC || headerIn->version != DB_VERSION)
	{
		printf("Bad header\n");
		return STATUS_ERROR;
	}

	if (headerIn->count == 0)
	{
		printf("No employees");
		return STATUS_ERROR;
	}
	
	ssize_t employeesSize = sizeof(struct employee_t) * headerIn->count;
	struct employee_t* employees = (struct employee_t *)xmalloc(employeesSize);
	if (employees == NULL)
	{
		perror("malloc");
		return STATUS_ERROR;
	}
	
	if (read(fd, employees, employeesSize) < employeesSize)
	{
		perror("read employees");
		free(*employeesOut);
		return STATUS_ERROR;
	}

	for (int i = 0; i < headerIn->count; i++)
	{
		employees[i].hours = ntohl(employees[i].hours);
	}
	*employeesOut = employees;
	return STATUS_SUCCESS;
}

int add_employee(struct dbheader_t *headerIn, struct employee_t *employeesIn, char* addStr)
{
	//strtok would be about 1000x simpler but that's no fun
	//we got plenty of memory allocated in structs, should be fine
	if (addStr == NULL || addStr[0] == '\0')
	{
		printf("Bad add string\n");
		return STATUS_ERROR;
	}

	if (headerIn == NULL || employeesIn == NULL)
	{
		printf("Error adding employee");
		return STATUS_ERROR;
	}

	char *name = (char*) &(employeesIn[headerIn->count - 1].name);
	char *addr = (char*) &(employeesIn[headerIn->count - 1].address);
	unsigned int *hours = &(employeesIn[headerIn->count - 1].hours);

	bzero(name, NAME_LEN);
	bzero(name, ADDRESS_LEN);

	ptrdiff_t seek = 0;
	char *start = addStr;

	while (start[seek] != ',' && start[seek] != '\0')
	{
		seek++;
		if (seek == NAME_LEN)
		{
			printf("Bad add string\n");
			return STATUS_ERROR;
		}
	}
	if (start[seek] == '\0')
	{
		printf("Bad add string\n");
		return STATUS_ERROR;
	}
	memcpy(name, start, seek < NAME_LEN - 1 ? seek : NAME_LEN - 1);
	start = &(start[seek + 1]);
	seek = 0;

	while (start[seek] != ',' && start[seek] != '\0')
	{
		seek++;
		if (seek == ADDRESS_LEN)
		{
			printf("Bad add string\n");
			return STATUS_ERROR;
		}
	}
	if (start[seek] == '\0')
	{
		printf("Bad add string\n");
		return STATUS_ERROR;
	}	
	memcpy(addr, start, seek < ADDRESS_LEN - 1 ? seek : ADDRESS_LEN - 1);

	//what about if addStr is not null terminated
	const int maxDigits = 20; //should be enough for any unsigned int on any system
	char hourBuf[maxDigits];
	bzero(hourBuf, maxDigits);
	strncpy((char *)hourBuf, &(start[seek + 1]), maxDigits - 1); //always leave a null byte
	*hours = (unsigned int)strtol(hourBuf, NULL, 10);

	return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *headerIn, struct employee_t *employees)
{
	if (fd < 0 || headerIn == NULL)
	{
		printf("Can't output file\n");
		return STATUS_ERROR;
	}

	if (headerIn->magic != HEADER_MAGIC || headerIn->version != DB_VERSION)
	{
		printf("Bad header\n");
		return STATUS_ERROR;
	}
	int count = headerIn->count;

	headerIn->magic = htonl(headerIn->magic);
	headerIn->version = htons(headerIn->version);
	headerIn->count = htons(headerIn->count);
	headerIn->filesize = htonl(
			sizeof(struct dbheader_t) + 
			sizeof(struct employee_t) * count);
	lseek(fd, 0, SEEK_SET);
	if (write(fd, headerIn, sizeof(struct dbheader_t)) < (ssize_t)sizeof(struct dbheader_t))
	{
		perror("write header");
		return STATUS_ERROR;
	}

	if (employees == NULL)
	{
		if (count == 0)
		{
			//no employees to write
			return STATUS_SUCCESS;
		}
		printf("No employees\n");
		return STATUS_ERROR;
	}

	for (int i = 0; i < count; i++)
	{
		employees[i].hours = htonl(employees[i].hours);
		if (write(fd, &employees[i], sizeof(struct employee_t)) < sizeof(struct employee_t))
		{
			perror("write employees");
			return STATUS_ERROR;
		}
	}

	return STATUS_SUCCESS;
}
