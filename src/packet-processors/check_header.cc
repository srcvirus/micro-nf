#include "check_header.h"
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_memcpy.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include "common/protocol_header_offset.h"
#include "common/checksum.h"
#include <iostream>

CheckHeader::~CheckHeader() {
  _cpu_ctr.get_stdev();
}

inline void CheckHeader::Init(const PacketProcessorConfig& pp_config) {
  num_ingress_ports_ = pp_config.num_ingress_ports();
  num_egress_ports_ = pp_config.num_egress_ports();
  instance_id_ = pp_config.instance_id();
  assert(num_ingress_ports_ == 1);
  assert(num_egress_ports_ == 1);
  for (int i = 0; i < num_ingress_ports_; ++i)
    ingress_ports_.emplace_back(nullptr);
  for (int j = 0; j < num_egress_ports_; ++j)
    egress_ports_.emplace_back(nullptr);
  PacketProcessor::ConfigurePorts(pp_config, this);
}

inline void CheckHeader::Run() {
  rx_pkt_array_t rx_packets;
  rx_pkt_array_t tx_packets;
  register uint16_t i = 0;
  uint16_t num_rx = 0;
  uint16_t num_tx = 0;
  uint8_t* mdata_ptr = nullptr;
  struct ProtocolHeaderOffset header_offsets;
  struct ether_hdr* eth = nullptr;
  struct ipv4_hdr* ip = nullptr;
  struct tcp_hdr* tcp = nullptr;
  const uint8_t* kHeaderOffsetPtr =
      reinterpret_cast<const uint8_t*>(header_offsets.data);
  const uint16_t kEtherTypeIPv4 = rte_cpu_to_be_16(0x0800);
  unsigned hlen;

  while (true) {

    num_rx = ingress_ports_[0]->RxBurst(rx_packets);
	num_tx = 0;

    for (i = 0; i < num_rx; ++i) {
      rte_prefetch0(rx_packets[i]->buf_addr);
      eth = rte_pktmbuf_mtod(rx_packets[i], struct ether_hdr*);
      header_offsets.eth_hdr_addr = reinterpret_cast<uint64_t>(eth);
      header_offsets.ip_hdr_addr =
          header_offsets.eth_hdr_addr + sizeof(struct ether_hdr);
      ip = reinterpret_cast<struct ipv4_hdr*>(header_offsets.ip_hdr_addr);
      header_offsets.transport_hdr_addr =
          header_offsets.ip_hdr_addr + sizeof(struct ipv4_hdr);

      // check the whole packet length, if smaller than
      // the ip header field, then drop it


      // check packet header length field, should be
      // bigger than the ip header
      hlen = (ip->version_ihl & 0xf) << 2;
      if (hlen < sizeof(ipv4_hdr)) {
          printf("drop for hdr length\n");
          rte_pktmbuf_free(rx_packets[i]);
          continue;
      }
      // check ip version, drop if version is not 4
      if ((ip->version_ihl & 0xf0) != 0x40){
          printf("drop for ip version\n");
          printf("version is: %d", (ip->version_ihl & 0xf0));
          rte_pktmbuf_free(rx_packets[i]);
          continue;
      }
      // check the total length, it should be larger
      // than the header length
      if (ntohs(ip->total_length) < hlen) {
          printf("drop for TOTAL length\n");
          rte_pktmbuf_free(rx_packets[i]);
          continue;
      }

      // check checksum
      if (ip_check_sum(reinterpret_cast<unsigned char*>(ip), hlen)) {
          printf("drop for checksum length\n");
          rte_pktmbuf_free(rx_packets[i]);
          continue;
      }

      if (likely(ip->next_proto_id == IPPROTO_TCP)) {
        tcp = reinterpret_cast<struct tcp_hdr*>(
            header_offsets.transport_hdr_addr);
        header_offsets.payload_addr =
            header_offsets.transport_hdr_addr + tcp->data_off;
      } else if (ip->next_proto_id == IPPROTO_UDP) {
        header_offsets.payload_addr =
            header_offsets.transport_hdr_addr + sizeof(struct udp_hdr);
      } else {
        header_offsets.payload_addr = 0;
      }
      mdata_ptr = reinterpret_cast<uint8_t*>(
          reinterpret_cast<unsigned long>(rx_packets[i]) +
          sizeof(struct rte_mbuf));
      rte_mov32(mdata_ptr, kHeaderOffsetPtr);

	  // copy right packets to tx array
	  tx_packets[num_tx++] = rx_packets[i];

    } 
    
    num_tx = egress_ports_[0]->TxBurst(rx_packets, num_tx);
  }
}

void CheckHeader::FlushState() {}
void CheckHeader::RecoverState() {}
