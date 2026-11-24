#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/atomic.h>
#include <linux/string.h>
#include <net/cfg80211.h>
#include <net/mac80211.h>




#define DRV_NAME "SOFTMAC"

/* Global driver structures */
static struct ieee80211_hw *wlan_hw;
static struct platform_device *wlan_pdev;

/* Driver private data */
struct wlan_priv
{
    unsigned int tx_packets;
    unsigned int rx_packets;
    unsigned int tx_bytes;
};

/* Start WLAN hardware */
static int wlan_start(struct ieee80211_hw *hw)
{
    pr_info("WLAN: Hardware started\n");

    return 0;
}

/* Stop WLAN hardware */
static void wlan_stop(struct ieee80211_hw *hw, bool suspend)
{
    pr_info("WLAN: Hardware stopped\n");
}

/* Add wireless interface */
static int wlan_add_interface(struct ieee80211_hw *hw,
                              struct ieee80211_vif *vif)
{
    pr_info("WLAN: Interface added\n");

    pr_info("WLAN: Interface type = %d\n", vif->type);

    return 0;
}

/* Remove wireless interface */
static void wlan_remove_interface(struct ieee80211_hw *hw,
                                  struct ieee80211_vif *vif)
{
    pr_info("WLAN: Interface removed\n");

    pr_info("WLAN: Interface type = %d\n", vif->type);
}

/* Transmit packet and perform RX loopback */
static void wlan_transmit(struct ieee80211_hw *hw,
                          struct ieee80211_tx_control *control,
                          struct sk_buff *skb)
{
    struct wlan_priv *priv = hw->priv;
    struct ieee80211_hdr *hdr;
    struct ieee80211_rx_status *status;
    struct sk_buff *rx_skb;

    unsigned int len;
    int hdr_len;

    /* Validate minimum frame length */
    if (skb->len < 2) {
        ieee80211_free_txskb(hw, skb);
        return;
    }

    /* Identify IEEE 802.11 frame */
    hdr = (struct ieee80211_hdr *)skb->data;

    /* Accept only data frames */
    if (!ieee80211_is_data(hdr->frame_control)) {
        ieee80211_free_txskb(hw, skb);
        return;
    }

    /* Calculate IEEE 802.11 header length */
    hdr_len = ieee80211_hdrlen(hdr->frame_control);

    if (skb->len < hdr_len) {
        ieee80211_free_txskb(hw, skb);
        return;
    }

    len = skb->len;

    pr_info("WLAN: DATA frame received\n");

    /* Copy packet for RX loopback */
    rx_skb = skb_copy(skb, GFP_ATOMIC);

    if (!rx_skb) {
        ieee80211_free_txskb(hw, skb);
        return;
    }

	if (skb->len > hdr_len) {
           pr_info( "Payload: %s\n", skb->data + hdr_len);


	}
    pr_info("WLAN: SKB copied for RX loopback\n");
    /* Configure RX metadata */
    status = IEEE80211_SKB_RXCB(rx_skb);

    memset(status, 0, sizeof(*status));

    status->freq = 2412;
    status->band = NL80211_BAND_2GHZ;
    status->signal = -40;
    status->rate_idx = 0;

    pr_info("WLAN: RX metadata configured\n");

    /* Pass packet to mac80211 RX path */
    pr_info("WLAN: Sending packet to RX path\n");

    ieee80211_rx(hw, rx_skb);

    priv->rx_packets++;

    /* Update network device TX statistics */
    if (skb->dev)
        dev_sw_netstats_tx_add(skb->dev, 1,len);

    /* Update driver TX statistics */
    priv->tx_packets++;
    priv->tx_bytes += len;

    pr_info("WLAN: TX packets = %u\n", priv->tx_packets);
    pr_info("WLAN: TX bytes = %u\n", priv->tx_bytes);

    /* Report TX completion to mac80211 */

    pr_info("WLAN: Reporting TX completion\n");

    ieee80211_tx_status_irqsafe(hw, skb);
}

/* Configure WLAN hardware */
static int wlan_configure(struct ieee80211_hw *hw,
                          int radio_idx,
                          u32 changed)
{
//Channel,Change transmit power, power-saving mode,Rates
    return 0;
}

/* Configure receive frame filter */
static void wlan_configure_rx_filter(struct ieee80211_hw *hw,
                                     unsigned int changed_flags,
                                     unsigned int *total_flags,
                                     u64 multicast)
{
    *total_flags = 0;
}

/* mac80211 operations */
static const struct ieee80211_ops wlan_ops = {

    .start            = wlan_start,
    .stop             = wlan_stop,

    .add_interface    = wlan_add_interface,
    .remove_interface = wlan_remove_interface,

    .tx               = wlan_transmit,

    .config           = wlan_configure,
    .configure_filter = wlan_configure_rx_filter,

    .wake_tx_queue    = ieee80211_handle_wake_tx_queue,

    .add_chanctx      = ieee80211_emulate_add_chanctx,
    .remove_chanctx   = ieee80211_emulate_remove_chanctx,
    .change_chanctx   = ieee80211_emulate_change_chanctx,
};

/* Supported 2.4 GHz channel */
static struct ieee80211_channel wlan_channels_2ghz[] = {
    {
        .band = NL80211_BAND_2GHZ,
        .center_freq = 2412,
        .hw_value = 1,
    },
};

/* Supported rates */
static struct ieee80211_channel wlan_channels_2ghz[] = {
    {
        .band = NL80211_BAND_2GHZ,
        .center_freq = 2412,
        .hw_value = 1,
    },
    {
        .band = NL80211_BAND_2GHZ,
        .center_freq = 2437,
        .hw_value = 6,
    },
    {
        .band = NL80211_BAND_2GHZ,
        .center_freq = 2462,
        .hw_value = 11,
    },
};
/* Supported 2.4 GHz band */
static struct ieee80211_supported_band wlan_band_2ghz = {

    .channels = wlan_channels_2ghz,
    .n_channels = ARRAY_SIZE(wlan_channels_2ghz),

    .bitrates = wlan_rates_2ghz,
    .n_bitrates = ARRAY_SIZE(wlan_rates_2ghz),

    .ht_cap.ht_supported = false,
};

/* WLAN driver initialization */
static int __init wlan_driver_init(void)
{
    int ret;
    struct wlan_priv *priv;
    u8 mac_addr[6] = {0x0A, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F};

    pr_info("WLAN: Driver initialization started\n");

    /* Allocate mac80211 hardware */
    wlan_hw = ieee80211_alloc_hw(sizeof(struct wlan_priv), &wlan_ops);

    if (!wlan_hw) {
        pr_err("WLAN: Hardware allocation failed\n");
        return -ENOMEM;
    }

    /* Initialize private driver data */
    priv = wlan_hw->priv;

    priv->tx_packets = 0;
    priv->rx_packets = 0;
    priv->tx_bytes = 0;

    /* Register software platform device */
    wlan_pdev = platform_device_register_simple(
                    DRV_NAME, -1, NULL, 0);

    if (IS_ERR(wlan_pdev)) {

        ret = PTR_ERR(wlan_pdev);
        wlan_pdev = NULL;

        ieee80211_free_hw(wlan_hw);
        wlan_hw = NULL;

	pr_err("WLAN: Platform device registration failed: %d\n",
			ret);

        return ret;
    }

    /* Associate wiphy with platform device */
    SET_IEEE80211_DEV(wlan_hw, &wlan_pdev->dev);

    /* Set permanent MAC address */
    SET_IEEE80211_PERM_ADDR(wlan_hw, mac_addr);

    /* Configure supported interface modes */
    wlan_hw->wiphy->interface_modes =
        BIT(NL80211_IFTYPE_STATION) |
        BIT(NL80211_IFTYPE_ADHOC) |
        BIT(NL80211_IFTYPE_AP) |
        BIT(NL80211_IFTYPE_MONITOR);

    /* Configure supported frequency band */
    wlan_hw->wiphy->bands[NL80211_BAND_2GHZ] =
        &wlan_band_2ghz;

    /* Enable monitor interface support */
    ieee80211_hw_set(wlan_hw, WANT_MONITOR_VIF);

    pr_info("WLAN: Registering mac80211 hardware\n");

    /* Register hardware with mac80211 */
    ret = ieee80211_register_hw(wlan_hw);

    if (ret) {

        pr_err("WLAN: Hardware registration failed: %d\n", ret);

        ieee80211_free_hw(wlan_hw);
        wlan_hw = NULL;

        platform_device_unregister(wlan_pdev);
        wlan_pdev = NULL;

        return ret;
    }

    pr_info("WLAN: Driver initialization completed\n");

    return 0;
}

/* WLAN driver cleanup */
static void __exit wlan_driver_exit(void)
{
    pr_info("WLAN: Driver cleanup started\n");

    /* Unregister and free mac80211 hardware */
    if (wlan_hw) {

        ieee80211_unregister_hw(wlan_hw);

        ieee80211_free_hw(wlan_hw);
        wlan_hw = NULL;
    }

    /* Unregister platform device */
    if (wlan_pdev) {

        platform_device_unregister(wlan_pdev);
        wlan_pdev = NULL;
    }

    pr_info("WLAN: Driver cleanup completed\n");
}

/* Module entry and exit */
module_init(wlan_driver_init);
module_exit(wlan_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PRUDHVI");
MODULE_DESCRIPTION("Software-only mac80211 SoftMAC WLAN Driver");

