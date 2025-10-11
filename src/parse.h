#pragma once

#define HEADER_MAGIC 0x4c4c4144
#define NAME_LEN 256
#define ADDRESS_LEN 256

#define DB_VERSION 1
#define MAX_EMPLOYEE_COUNT 123456 //arbitrary for now
#define FAIL 1
#define OK 0

struct dbheader_s {
        unsigned int magic;
        unsigned short version;
        unsigned short count;
        unsigned int filesize;
};

struct employee_s {
        char name[NAME_LEN];
        char address[ADDRESS_LEN];
        unsigned int hours;
};

int create_db_header(struct dbheader_s **headerOut);
int validate_db_header(int fd, struct dbheader_s **headerOut);
int read_employees(int fd, struct dbheader_s * headerIn, struct employee_s **employeesOut);
int output_file(int fd, struct dbheader_s * headerIn, struct employee_s *employees);
