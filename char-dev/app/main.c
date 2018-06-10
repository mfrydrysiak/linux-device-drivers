#include <stdio.h>

int main()
{
    const char * driver_path = "/dev/mfchar";
    FILE * fd = NULL;

    fd = fopen(driver_path, "rw");
    if (fd == NULL) {
        printf("Could not open the device!\n");
        return 1;
    }
    printf("Device successfully opened.\n");

    fclose(fd);
    printf("Device closed.\n");

    return 0;
}
