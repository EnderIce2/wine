
#include "esync.h"

int do_esync(void)
{
    return 0;
}

/* Wake up a specific fd. */
void esync_wake_fd( int fd )
{
    static const unsigned long long value = 1;

    if (write( fd, &value, sizeof(value) ) == -1)
        perror( "esync: write" );
}
