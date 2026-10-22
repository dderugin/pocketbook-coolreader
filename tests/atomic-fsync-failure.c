/* Loaded only into a disposable ARM/QEMU regression subprocess. */
#include <errno.h>
#include <unistd.h>
int fsync(int fd) {
    (void)fd;
    errno = EIO;
    return -1;
}
