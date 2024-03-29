#include "classfy_http.h"
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_memcpy.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <rte_byteorder.h>

inline void ClassfyHTTP::Init(const PacketProcessorConfig& pp_config) {
  num_ingress_ports_ = pp_config.num_ingress_ports();
  num_egress_ports_ = pp_config.num_egress_ports();
  instance_id_ = pp_config.instance_id();
  assert(num_ingress_ports_ == 1);
  assert(num_egress_ports_ == 2);
  fflush(stdout);
  for (int i = 0; i < num_ingress_ports_; ++i)
    ingress_ports_.emplace_back(nullptr);
  for (int j = 0; j < num_egress_ports_; ++j)
    egress_ports_.emplace_back(nullptr);
  PacketProcessor::ConfigurePorts(pp_config, this);
  fflush(stdout);
}

void
ClassfyHTTP::Run() {
    rx_pkt_array_t rx_packets;
    rx_pkt_array_t tx_packets_1;
    rx_pkt_array_t tx_packets_2;
    uint16_t num_rx = 0;
    uint16_t num_tx_1 = 0;
    uint16_t num_tx_2 = 0;
    register int i = 0;
    struct ether_hdr* eth = nullptr;
    struct ipv4_hdr* ip = nullptr;
    struct tcp_hdr* tcp = nullptr;
    char* payload = nullptr;
    uint32_t offset = 0;

    std::string pattern = "(GET )(https?:\\/\\/)([a-z\\.\\/]*)( )(HTTP\\/1.[01] )(Host:)( )(.*)";
    std::regex url_regex(pattern);
 
    while (true) {
        num_rx = ingress_ports_[0]->RxBurst(rx_packets);
        num_tx_1 = 0;
        num_tx_2 = 0;

		for (i = 0; i < num_rx; ++i) {
            head = nullptr;
            tail = nullptr;
            eth = rte_pktmbuf_mtod(rx_packets[i], struct ether_hdr*);
            ip  = reinterpret_cast<ipv4_hdr*>(eth + 1);
            tcp = reinterpret_cast<tcp_hdr*>(ip + 1);
            payload = reinterpret_cast<char*>(tcp) + 16;
            offset = sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + (tcp->data_off & 0xf0) >> 2;

            /* find the head and tail of the url in the packet here */
            head = payload;
            //tail = payload + rx_packets[i]->pkt_len - offset; // original 15
            tail = payload + 50; 

            std::string url(head, tail);
            if (std::regex_match(url, url_regex)) {
				tx_packets_1[num_tx_1++] = rx_packets[i];
            } else { // does not match
				tx_packets_2[num_tx_2++] = rx_packets[i];
            }
        }
        num_tx_1 = egress_ports_[0]->TxBurst(tx_packets_1, num_tx_1);
        num_tx_2 = egress_ports_[0]->TxBurst(tx_packets_2, num_tx_2);
    }
}

void ClassfyHTTP::FlushState() {}
void ClassfyHTTP::RecoverState() {}
