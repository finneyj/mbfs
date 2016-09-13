// mbfs.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string.h>

#include "MicroBitFileSystem.h"
#include "MicroBitFile.h"
#include "MicroBitFlash.h"

MicroBitFileSystem mbfs;

void print_file(char *filename)
{
	int fd = mbfs.open(filename, MB_READ);
	uint8_t buf[10000];

	mbfs.read(fd, buf, 10000);
	mbfs.close(fd);
	printf("<PRINTFILE>\n  %s\n</PRINTFILE>\n", (char *)buf);
}


void write2_read2_test()
{
	char *message = "Hello World!";
	char *message2 = "Goodbye Cruel World!";
	int fd;

	fd = mbfs.open("test1", MB_WRITE | MB_CREAT);

	mbfs.write(fd, (uint8_t *)message, strlen(message) + 1);
	mbfs.close(fd);

	fd = mbfs.open("test", MB_WRITE | MB_CREAT);

	mbfs.write(fd, (uint8_t *)message2, strlen(message2) + 1);
	mbfs.close(fd);

	print_file("test");
	print_file("test1");
}

void write_read_n_test(int n)
{
	char message[32];
	char filename[16];

	for (int i = 0; i < n;  i++)
	{
		sprintf(message, "Hello World -%d-", i);
		sprintf(filename, "test%d", i);

		int fd;

		fd = mbfs.open(filename, MB_WRITE | MB_CREAT);
		mbfs.write(fd, (uint8_t *)message, strlen(message) + 1);
		mbfs.close(fd);

		print_file(filename);
	}
}

void write_read_test(char *name)
{
	char *message = "Hello World!";
	int fd;
	
	fd = mbfs.open(name, MB_WRITE | MB_CREAT);

	mbfs.write(fd, (uint8_t *)message, strlen(message)+1);
	mbfs.close(fd);

	print_file(name);
}

void seek_test(char *name, int offset)
{
	char *message = "Hello World!";
	uint8_t buf[10000];
	int fd;

	fd = mbfs.open(name, MB_READ);

	mbfs.seek(fd, offset, MB_SEEK_SET);

	int len = mbfs.read(fd, buf, 10000);

	if (len >= 0)
		buf[len] = 0;

	mbfs.close(fd);

	printf("<SEEKTEST>:\n  %s\n</SEEKTEST>", buf);
}


void write_test(char *name, int size, int terminate = 1)
{
	uint8_t buf[10000];
	int fd;

	char c = 'a';

	for (int i = 0; i < size; i++)
	{
		buf[i] = c++;
		if (c > 'z')
			c = 'a';
	}

	if (terminate)
		buf[size - 1] = 0;

	fd = mbfs.open(name, MB_WRITE | MB_CREAT);

	mbfs.write(fd, buf, size);
	mbfs.close(fd);
}

void rewrite_test(char *name, int size, int terminate = 1)
{
	uint8_t buf[10000];
	int fd;

	char c = '0';

	for (int i = 0; i < size; i++)
	{
		buf[i] = c++;
		if (c > '9')
			c = '0';
	}

	if (terminate)
		buf[size - 1] = 0;

	fd = mbfs.open(name, MB_WRITE | MB_CREAT);

	mbfs.write(fd, buf, size);
	mbfs.close(fd);
}


void append_test(char *name, int size, int terminate = 1)
{
	uint8_t buf[10000];
	int fd;

	char c = '0';

	for (int i = 0; i < size; i++)
	{
		buf[i] = c++;
		if (c > '9')
			c = '0';
	}

	if (terminate)
		buf[size - 1] = 0;

	fd = mbfs.open(name, MB_WRITE | MB_APPEND);

	mbfs.write(fd, buf, size);
	mbfs.close(fd);
}


void write_delete_test()
{
	char *message = "Hello World!";
	int fd;

	fd = mbfs.open("test", MB_WRITE | MB_CREAT);

	mbfs.write(fd, (uint8_t *)message, strlen(message) + 1);
	mbfs.close(fd);

	mbfs.remove("test");
}

void reopen_test(char *filename)
{
	int fd, fd2;

	fd = mbfs.open(filename, MB_WRITE | MB_CREAT);

	fd2 = mbfs.open(filename, MB_READ);

	mbfs.close(fd);

}


int main()
{
	//write_read_n_test(28);

	//mbfs.remove("test0");
	//mbfs.remove("test1");
	//mbfs.remove("test8");

	// Fill the file system. Leave only two block unallocated.
	write_test("poop", 256, 1);
	for (int i = 0; i < 154; i++)
		append_test("poop", 256, 0);

	write_test("file1", 200, 1);
	write_test("file2", 200, 1);

	mbfs.debugFAT();
	mbfs.debugRootDirectory();

	mbfs.remove("file1");

	mbfs.debugFAT();
	mbfs.debugRootDirectory();

	write_test("file3", 200, 1);

	mbfs.debugFAT();
	mbfs.debugRootDirectory();

	return 0;
}