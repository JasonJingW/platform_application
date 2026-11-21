#include "network.h"
#include "network_config.h"
#include "project_config.h"
#include "bsp_eth.h"
#include "lwip/etharp.h"
#include "lwip/init.h"
#include "lwip/stats.h"
#include "lwip/timeouts.h"
#include "lwip/udp.h"
#include "netif/ethernet.h"

#define NETWORK_MTU (1500U)
#define NETWORK_RX_BUDGET (8U)
#define NETWORK_LINK_POLL_MS (200U)
#define NETWORK_UDP_MAX_PAYLOAD (1472U)

#if ICMP_STATS
static void network_report_icmp(void)
{
    static struct stats_proto previous;
    const struct stats_proto *current = &lwip_stats.icmp;
    if (previous.recv != current->recv || previous.xmit != current->xmit ||
        previous.chkerr != current->chkerr)
    {
        RLOG_INFO("Ethernet ICMP rx=%u tx=%u checksum_errors=%u", (unsigned)current->recv,
                  (unsigned)current->xmit, (unsigned)current->chkerr);
        previous = *current;
    }
}
#endif

static struct netif network_interface;
static uint8_t network_mac[BSP_ETH_MAC_SIZE];
static uint8_t network_tx_frame[BSP_ETH_FRAME_SIZE];
static uint32_t network_link_checked;
static bool network_stack_ready;

u32_t sys_now(void)
{
    return HAL_GetTick();
}

void network_lwip_assert(const char *message)
{
    (void)message;
    RLOG_ERR("lwIP assertion: %s", message);
    vTaskSuspendAll();
    for (;;)
    {
        __WFI();
    }
}

static err_t network_output(struct netif *interface, struct pbuf *packet)
{
    (void)interface;
    const u16_t length = packet->tot_len;
    if (length > sizeof(network_tx_frame) ||
        pbuf_copy_partial(packet, network_tx_frame, length, 0U) != length)
    {
        return ERR_BUF;
    }
    return bsp_eth_send(network_tx_frame, length) == HAL_OK ? ERR_OK : ERR_IF;
}

static err_t network_interface_init(struct netif *interface)
{
    interface->name[0] = 'e';
    interface->name[1] = 'n';
    interface->output = etharp_output;
    interface->linkoutput = network_output;
    interface->mtu = NETWORK_MTU;
    interface->hwaddr_len = sizeof(network_mac);
    memcpy(interface->hwaddr, network_mac, sizeof(network_mac));
    interface->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;
    return ERR_OK;
}

static void network_udp_echo(void *argument, struct udp_pcb *pcb, struct pbuf *packet,
                             const ip_addr_t *address, u16_t port)
{
    (void)argument;
    /* Only answer packets addressed directly to this interface. */
    if (packet->tot_len <= NETWORK_UDP_MAX_PAYLOAD && port != 0U && !ip_addr_isany(address) &&
        !ip_addr_isbroadcast(address, &network_interface) && !ip_addr_ismulticast(address) &&
        ip_addr_cmp(ip_current_dest_addr(), netif_ip_addr4(&network_interface)))
    {
        udp_sendto(pcb, packet, address, port);
    }
    pbuf_free(packet);
}

bool network_init(void)
{
    ip4_addr_t address;
    ip4_addr_t netmask;
    ip4_addr_t gateway;
    if (!ip4addr_aton(NETWORK_IP_ADDRESS, &address) || !ip4addr_aton(NETWORK_NETMASK, &netmask) ||
        !ip4addr_aton(NETWORK_GATEWAY, &gateway) || bsp_eth_init(network_mac) != HAL_OK)
    {
        return false;
    }
    if (!network_stack_ready)
    {
        lwip_init();
        network_stack_ready = true;
    }

    struct udp_pcb *pcb = udp_new_ip_type(IPADDR_TYPE_V4);
    if (pcb == NULL)
    {
        return false;
    }
    if (udp_bind(pcb, &address, NETWORK_UDP_PORT) != ERR_OK ||
        netif_add(&network_interface, &address, &netmask, &gateway, NULL, network_interface_init,
                  ethernet_input) == NULL)
    {
        udp_remove(pcb);
        return false;
    }
    udp_recv(pcb, network_udp_echo, NULL);
    netif_set_default(&network_interface);
    netif_set_up(&network_interface);
    network_link_checked = HAL_GetTick() - NETWORK_LINK_POLL_MS;
    RLOG_INFO("Ethernet ready, IP %s, UDP echo %u", NETWORK_IP_ADDRESS, (unsigned)NETWORK_UDP_PORT);
    return true;
}

void network_process(void)
{
    const uint32_t now = HAL_GetTick();
    if (now - network_link_checked >= NETWORK_LINK_POLL_MS)
    {
        network_link_checked = now;
        const uint32_t speed = bsp_eth_poll_link();
        if (speed > 0U && !netif_is_link_up(&network_interface))
        {
            netif_set_link_up(&network_interface);
            RLOG_INFO("Ethernet link up (%u Mbit/s)", (unsigned)speed);
        }
        else if (speed == 0U && netif_is_link_up(&network_interface))
        {
            netif_set_link_down(&network_interface);
            RLOG_WAR("Ethernet link down");
        }
#if ICMP_STATS
        network_report_icmp();
#endif
    }

    for (unsigned count = 0U; count < NETWORK_RX_BUDGET; ++count)
    {
        uint16_t length;
        const uint8_t *frame = bsp_eth_receive(&length);
        if (frame == NULL)
        {
            break;
        }
        struct pbuf *packet = pbuf_alloc(PBUF_RAW, length, PBUF_POOL);
        if (packet != NULL)
        {
            if (pbuf_take(packet, frame, length) != ERR_OK ||
                network_interface.input(packet, &network_interface) != ERR_OK)
            {
                pbuf_free(packet);
            }
        }
    }
    sys_check_timeouts();
}
