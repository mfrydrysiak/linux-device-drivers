#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define ARRAY_COUNT(x) (sizeof(x)/sizeof(x[0]))
#define RX_BUFF_SIZE   128

int main()
{
    const char * driver_path = "/dev/mfchar";
    const char send_buffer[] = {"Hello Kernel!"};
    char read_buffer[RX_BUFF_SIZE];
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

    printf("Press ENTER...\n");
    getchar();

    printf("Attempting to read from the device.\n");
    count = read(fd, read_buffer, RX_BUFF_SIZE);
    if (count < 0)
        printf("Error on reading from the device!\n");
    else
        printf("Read from the device: %s\n", read_buffer);

    close(fd);
    printf("Device closed.\n");

    return 0;
}
