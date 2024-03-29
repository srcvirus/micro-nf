#ifndef _EGRESS_PORT_H_
#define _EGRESS_PORT_H_

#include <rte_mbuf.h>
#include <rte_memzone.h>

#include "../common/msstats.h"

#include <array>
#include <map>
#include <string>

#define TX_BURST_SIZE 64

typedef std::array<struct rte_mbuf*, TX_BURST_SIZE> tx_pkt_array_t;

class PacketProcessor;

class EgressPort {
 public:
  // Initializes the port. Since this is implementation specific, therefore,
  // each type of port should provide their own implementation of Init method.
  virtual void Init(std::map<std::string, std::string>& port_config, 
									PacketProcessor* owner_pp = nullptr) = 0;

  // Send a burst of packets (maximum TX_BURST_SIZE packets) out of this port.
  // packets contains the mbuf pointers that need to be sent. Return value is
  // the number of actual mbufs sent over this port.
  virtual int TxBurst(tx_pkt_array_t& packets, uint16_t burst_size = TX_BURST_SIZE) = 0;

  int port_id() const { return this->port_id_; }
  virtual ~EgressPort() {}

  static const std::string kConfPortId;
  static const std::string kConfRingId;

 protected:
  // An identifier assigned to this port. Identifier assignment is specific to
  // port and not necessarily globally unique.
  int port_id_;
	
	const struct rte_memzone *stat_mz;
	MSStats* micronf_stats;
  PacketProcessor* owner_packet_processor_;

};
#endif  // _EGRESS_PORT_H_
