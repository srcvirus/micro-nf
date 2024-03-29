#include "mark_and_forward_egress_port.h"

#include <rte_atomic.h>
#include <rte_mbuf.h>
#include <iostream>

void MarkAndForwardEgressPort::Init(
    std::map<std::string, std::string>& port_config,
    PacketProcessor* owner_pp) {
  this->port_id_ = std::stoi(port_config[EgressPort::kConfPortId]);
  this->tx_ring_ =
      rte_ring_lookup(port_config[EgressPort::kConfRingId].c_str());
  assert(this->tx_ring_ != nullptr);
  this->stat_mz = rte_memzone_lookup(MZ_STAT);
  this->micronf_stats = (MSStats*)this->stat_mz->addr;
  this->owner_packet_processor_ = owner_pp;
}

inline int MarkAndForwardEgressPort::TxBurst(tx_pkt_array_t& packets,
                                             uint16_t burst_size) {
  register uint16_t i = 0;

  // For each mbuf, increase the counter in the meta-data area before pushing
  // it to the next stage. rte_atomic* ensures atomic read and increase of
  // the shared counter.
  for (i = 0; i < burst_size; ++i) {
    char* mdata_ptr = MDATA_PTR(packets[i]);
    rte_atomic16_inc(reinterpret_cast<rte_atomic16_t*>(mdata_ptr));
  }

  uint16_t num_tx = rte_ring_enqueue_burst(
      this->tx_ring_, reinterpret_cast<void**>(packets.data()), burst_size,
      NULL);
  if (unlikely((num_tx < burst_size))) {
    //		this->micronf_stats->packet_drop[owner_packet_processor_->instance_id()][this->port_id_]
    //+=
    //			(burst_size - num_tx);
    for (i = num_tx; i < burst_size; ++i) {
      rte_pktmbuf_free(packets[i]);
    }
  }
  return num_tx;
}
