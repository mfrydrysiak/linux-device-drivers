#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define ARRAY_COUNT(x) (sizeof(x)/sizeof(x[0]))

int main()
{
    const char * driver_path = "/dev/mfchar";
    const char send_buffer[] = {"Hello Kernel!"};
    ssize_t count;
    int fd;

    fd = open(driver_path, O_RDWR);
    if (fd < 0) {
        printf("Could not open the device!\n");
        return 1;
    }
    printf("Device successfully opened.\n");

    count = write(fd, send_buffer, ARRAY_COUNT(send_buffer));
    printf("Written %ld characters to the device.\n", count);

    close(fd);
    printf("Device closed.\n");

    return 0;
}
