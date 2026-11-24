
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

unsigned long long read_counter(char *name)
{
    char path[100];
    unsigned long long bytes;

    sprintf(path,
        "/sys/class/net/wlan0/statistics/%s_bytes", name);

    FILE *fp = fopen(path, "r");

    if (fp == NULL)
        return 0;

    fscanf(fp, "%llu", &bytes);
    fclose(fp);

    return bytes;
}

int main()
{
    unsigned long long tx1, tx2, rx1, rx2;

    while (1)
    {
        tx1 = read_counter("tx");
        rx1 = read_counter("rx");

        sleep(1);

        tx2 = read_counter("tx");
        rx2 = read_counter("rx");

        printf("TX: %.2f Mbps | RX: %.2f Mbps\n",
            (tx2 - tx1) * 8.0 / 1000000,
            (rx2 - rx1) * 8.0 / 1000000);
    }

    return 0;
}

