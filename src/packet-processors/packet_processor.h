#ifndef _PACKET_PROCESSOR_H_
#define _PACKET_PROCESSOR_H_

#include <rte_memzone.h>

#include "../port/port.h"
#include "packet_processor_config.pb.h"
#include "../common/scale_bit_vector.h"

#include <memory>
#include <vector>

#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#define PATH_TO_SEM "/home/h4bian/aqua10/micro-nf-datapath/src/VSEM"
#define SEM_PROJECT_ID 100

// fixme
#define n_share_core_x 2

class PacketProcessor {
 public:
  PacketProcessor() : num_ingress_ports_(0), num_egress_ports_(0) {
	  this->scale_bits_mz = rte_memzone_lookup(MZ_SCALE);
	  this->scale_bits = (ScaleBitVector*) this->scale_bits_mz->addr;
	}

  // Initialize the packet processor object with the provided configuration.
  virtual void Init(const PacketProcessorConfig& pp_config) = 0;

  // Run is expected to be a blocking method that iterates over the ingress
  // ports, reads packet batches, performs processing and then writes them to
  // appropriate egress ports if applicable.
  virtual void Run() = 0;

  virtual void FlushState() = 0;
  virtual void RecoverState() = 0;
  const int instance_id() const { return instance_id_; }
  inline uint16_t num_ingress_ports() const { return this->num_ingress_ports_; }
  inline uint16_t num_egress_ports() const { return this->num_egress_ports_; }
  std::vector<std::unique_ptr<IngressPort>>& ingress_ports() {
    return ingress_ports_;
  }
  std::vector<std::unique_ptr<EgressPort>>& egress_ports() {
    return egress_ports_;
  }

  virtual ~PacketProcessor(){};

 protected:
  // Configure the ports of this packet processor according to provided
  // configuration. This will be a method private to each PacketProcessor
  // implementation. 
  void ConfigurePorts(const PacketProcessorConfig& pp_config, 
			PacketProcessor* owner_pp = nullptr);
  int lookup_vsem();
  int wait_vsem( int set_id, int idx );
  int post_vsem( int set_id, int idx ); 

  int instance_id_;
  uint16_t num_ingress_ports_;
  uint16_t num_egress_ports_;
  std::vector<std::unique_ptr<IngressPort>> ingress_ports_;
  std::vector<std::unique_ptr<EgressPort>> egress_ports_;

  const struct rte_memzone *scale_bits_mz;
  ScaleBitVector *scale_bits;

  int share_core_;
  int sem_enable_;
  int cpu_id_; 
  int sem_set_id_;
  int share_p_idx_;   // my index among processes sharing the cpu
  int share_np_idx_;   // index of next process sharing the cpu.

  static const std::string shareCoreFlag;
  static const std::string semaphoreFlag;
  static const std::string cpuId;
};

// template <> class PacketProcessor <RteIngressPort, RteEgressPort> {
// };

// template <> class PacketProcessor <NullIngressPort, NullEgressPort> {
// };
#endif  // _PACKET_PROCESSOR_H_
