#include "mac_swapper.h"

#include <algorithm>
#include <assert.h>
#include <google/protobuf/map.h>
#include <rte_ether.h>
#include <rte_prefetch.h>
#include <stdlib.h>
#include <rte_cycles.h>
#include <iostream>
#include <inttypes.h>
#include <rte_log.h>

inline void MacSwapper::Init(const PacketProcessorConfig& pp_config) {
   num_ingress_ports_ = pp_config.num_ingress_ports();
   num_egress_ports_ = pp_config.num_egress_ports();
   instance_id_ = pp_config.instance_id();

   // First, initialize the list of ingress and egress ports.
   int i = 0;
   for (i = 0; i < this->num_ingress_ports_; ++i) {
      ingress_ports_.emplace_back(nullptr);
   }
   for (i = 0; i < this->num_egress_ports_; ++i) {
      egress_ports_.emplace_back(nullptr);
   }
   
   // Retrieve pp_params config
   auto pp_param_map = pp_config.pp_parameters();
   auto it = pp_param_map.find( PacketProcessor::compLoad );
   if ( it !=  pp_param_map.end() )
      comp_load_ = it->second;

   it = pp_param_map.find( PacketProcessor::shareCoreFlag );
   if ( it != pp_param_map.end() )
      share_core_ = it->second;
   
   it = pp_param_map.find( "debug" );
   if ( it !=  pp_param_map.end() )
      debug_ = it->second;

   it = pp_param_map.find( PacketProcessor::yieldAfterBatch );
   if ( it !=  pp_param_map.end() )
      yield_after_kbatch_ = it->second;

   it = pp_param_map.find( PacketProcessor::kNumPrefetch );
   if ( it != pp_param_map.end() )
      k_num_prefetch_ = it->second;
	
   it = pp_param_map.find( PacketProcessor::iterPayload );
   if ( it != pp_param_map.end() )
      iter_payload_ = it->second;

   RTE_LOG( INFO, PMD, "k_num_prefetch_ : %d\n", k_num_prefetch_);   

   PacketProcessor::ConfigurePorts(pp_config, this);
}

inline void MacSwapper::Run() {
   rx_pkt_array_t rx_packets;
   register int16_t i = 0;
   struct ether_hdr* eth_hdr = nullptr;
   uint16_t num_rx = 0;
   int res = 0;
   uint32_t hit_count = 1;
   uint64_t start_ts, end_ts;
   const static int ar_size = 100;
   uint64_t diff_ts[ ar_size ];
   uint32_t sample_counter = 0;
   uint ts_idx = 0;

   int kNumPrefetch = 8;
   
   while ( true ) {
      num_rx = this->ingress_ports_[0]->RxBurst(rx_packets);
      // If num_rx == 0 -> yield
      // Otherwise, try again and until k consecutive hits and then yield
      if ( share_core_ ) {
         if ( hit_count == yield_after_kbatch_ ) {
            hit_count = 1;
            res = sched_yield();
            if ( unlikely( res == -1 ) ) {
               std::cerr << "sched_yield failed! Exitting." << std::endl;
               return;
            }
         }
         else
            hit_count++;
      }

      for (i = 0; i < num_rx && i < k_num_prefetch_; ++i)
         rte_prefetch_non_temporal(rte_pktmbuf_mtod(rx_packets[i], void*));
      for (i = 0; i < num_rx - k_num_prefetch_; ++i) {
         rte_prefetch_non_temporal(rte_pktmbuf_mtod(rx_packets[i + k_num_prefetch_], void*));
         eth_hdr = rte_pktmbuf_mtod(rx_packets[i], struct ether_hdr*);
         // Read num_bytes of tcp payload. add true add last to write to payload.
         //this->iterate_payload( eth_hdr, iter_payload_, true );
         std::swap(eth_hdr->s_addr.addr_bytes, eth_hdr->d_addr.addr_bytes);
      }
      for ( ; i < num_rx; ++i) {
         eth_hdr = rte_pktmbuf_mtod(rx_packets[i], struct ether_hdr*);
         // Read num_bytes of tcp payload. add true add last to write to payload.
         //this->iterate_payload( eth_hdr, iter_payload_, true );
         std::swap(eth_hdr->s_addr.addr_bytes, eth_hdr->d_addr.addr_bytes);
      }
      
      // Imitate the processing cost a batch of packets
      if ( num_rx > 0 ){
         imitate_processing( comp_load_ );
      }

      this->egress_ports_[0]->TxBurst(rx_packets, num_rx);
      for (i=0; i < num_egress_ports_; i++){
         if (this->scale_bits->bits[this->instance_id_].test(i)){
            // TODO  Change port to smart port.
            // this->scale_bits->bits[this->instance_id_].set(i, false);
         }
      }
   } 
}

inline void MacSwapper::FlushState() {}
inline void MacSwapper::RecoverState() {}
