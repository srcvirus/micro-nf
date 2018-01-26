#ifndef _PACKET_PROCESSOR_H_
#define _PACKET_PROCESSOR_H_

#include <rte_memzone.h>

#include "../port/port.h"
#include "packet_processor_config.pb.h"
#include "../common/scale_bit_vector.h"

#include <memory>
#include <vector>

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

  __inline__ uint64_t start_rdtsc() {
     unsigned int lo,hi;
     //preempt_disable();
     //raw_local_irq_save(_flags);

     __asm__ __volatile__ ("CPUID\n\t"
                           "RDTSC\n\t"
                           "mov %%edx, %0\n\t"
                           "mov %%eax, %1\n\t": "=r" (hi), "=r" (lo):: "%rax", "%rbx", "%rcx", "%rdx");
     return ((uint64_t)hi << 32) | lo;
  }

  __inline__ uint64_t end_rdtsc() {
     unsigned int lo, hi;

     __asm__ __volatile__ ("RDTSCP\n\t"
                           "mov %%edx, %0\n\t"
                           "mov %%eax, %1\n\t"
                           "CPUID\n\t": "=r" (hi), "=r" (lo):: "%rax", "%rbx", "%rcx", "%rdx");
     //raw_local_irq_save(_flags);
     //preempt_enable();
     return ((uint64_t)hi << 32) | lo;
  }

  // Imitating work load.
  // void imitate_processing( int load ) __attribute__((optimize("O0"))); 
  void __inline__ imitate_processing( int load ) __attribute__((optimize("O0"))){   
     // Imitate extra processing
     int n = 1000 * load;
     for ( int i = 0; i < n; i++ ) {
        int volatile r = 0;
        int volatile s = 999;
        r =  s * s;
     }
  }


  int instance_id_;
  uint16_t num_ingress_ports_;
  uint16_t num_egress_ports_;
  std::vector<std::unique_ptr<IngressPort>> ingress_ports_;
  std::vector<std::unique_ptr<EgressPort>> egress_ports_;

  const struct rte_memzone *scale_bits_mz;
  ScaleBitVector *scale_bits;

  int share_core_ = 0;
  int cpu_id_; 
  int comp_load_ = 0;
  int debug_ = 0;
  int yield_after_kbatch_ = 2;

  static const std::string shareCoreFlag;
  static const std::string cpuId;
  static const std::string compLoad;
  static const std::string yieldAfterBatch;
};

// template <> class PacketProcessor <RteIngressPort, RteEgressPort> {
// };

// template <> class PacketProcessor <NullIngressPort, NullEgressPort> {
// };
#endif  // _PACKET_PROCESSOR_H_
