#ifndef _EGRESS_PORT_H_
#define _EGRESS_PORT_H_

#include <array>

#define TX_BURST_SIZE 64

class EgressPort {
  public:
    // Send a burst of packets (maximum TX_BURST_SIZE packets) out of this port.
    // packets contains the mbuf pointers that need to be sent. Return value is
    // the number of actual mbufs sent over this port.
    virtual int TxBurst(std::array<struct rte_mbuf*, TX_BURST_SIZE>& packets) = 0;
    int port_id() const { return this->port_id_; }
  protected:
    // An identifier assigned to this port. Identifier assignment is specific to
    // port and not necessarily globally unique.
    unsigned int port_id_;
};
#endif // _EGRESS_PORT_H_
