#include <stdio.h>
#include <fcntl.h>


int main()
{
	int fd;
	fd = open("/dev/kyouko3",O_RDWR);
	close(fd);
	return 0;
}
