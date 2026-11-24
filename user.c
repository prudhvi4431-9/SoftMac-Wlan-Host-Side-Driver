#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <endian.h>

/* IEEE 802.11 three-address header */
struct ieee80211_header {
    uint16_t frame_control;
    uint16_t duration;
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];
    uint16_t sequence;
} __attribute__((packed));

int main(void)
{
    int sock;
    struct ifreq ifr;
    struct sockaddr_ll addr;

    const char *iface = "wlan0";

    unsigned char buffer[128] = {0};

    /* Radiotap header */
    unsigned char radiotap[8] = {
        0x00, 0x00, 0x08, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    const char payload[] = "Hello Dummy SoftMAC WLAN";

    struct ieee80211_header *hdr;

    /* Create raw packet socket */
    sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if (sock < 0) {
        perror("socket");
        return 1;
    }

    /* Get interface index */
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);

    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        close(sock);
        return 1;
    }

    /* Configure destination interface */
    memset(&addr, 0, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = ifr.ifr_ifindex;
    addr.sll_protocol = htons(ETH_P_ALL);

    /* Add radiotap header */
    memcpy(buffer, radiotap, sizeof(radiotap));

    /* Prepare 802.11 header */
    hdr = (struct ieee80211_header *)(buffer + sizeof(radiotap));

    hdr->frame_control = htole16(0x0008); /* DATA frame */
    hdr->duration = 0;

    /* Receiver MAC: wlan0 */
    const uint8_t receiver_mac[6] = {
	    0x0a, 0x1b, 0x2c, 0x3d, 0x4e, 0x5f
    };

    /* Transmitter MAC: Test device */
    const uint8_t transmitter_mac[6] = {
	    0x02, 0x11, 0x22, 0x33, 0x44, 0x55
    };

    /* BSSID: Test access point */
    const uint8_t bssid[6] = {
	    0x02, 0xaa, 0xbb, 0xcc, 0xdd, 0xee
    };

    memcpy(hdr->addr1, receiver_mac, 6);
    memcpy(hdr->addr2, transmitter_mac, 6);
    memcpy(hdr->addr3, bssid, 6);
    hdr->sequence = 0;

    /* Add payload */
    memcpy(buffer + sizeof(radiotap) + sizeof(*hdr),
		    payload, sizeof(payload) - 1);

    int total_len = sizeof(radiotap) +
	    sizeof(*hdr) +
	    sizeof(payload) - 1;

    /* Inject five DATA frames */
    for (int i = 0; i < 100000000; i++) {

        hdr->sequence = htole16(i << 4);

        if (sendto(sock, buffer, total_len, 0,
                   (struct sockaddr *)&addr,
                   sizeof(addr)) < 0) {
            perror("sendto");
            break;
	
        }


        printf("DATA frame %d sent\n", i + 1);
//        sleep(1);
    }

    close(sock);
    return 0;
}

