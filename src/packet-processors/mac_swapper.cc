#include "mac_swapper.h"

#include <algorithm>
#include <assert.h>
#include <google/protobuf/map.h>
#include <rte_ether.h>
#include <rte_prefetch.h>
#include <stdlib.h>
#include <rte_cycles.h>
#include <iostream>

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
   
   // Retrieve share cpu config
   share_core_ = pp_config.pp_parameters().find( 
         PacketProcessor::shareCoreFlag )->second;
   sem_enable_ = pp_config.pp_parameters().find(
         PacketProcessor::semaphoreFlag )->second;
   cpu_id_ = pp_config.pp_parameters().find( PacketProcessor::cpuId )->second;

   fprintf( stdout, "mac_swapper.cc: Id:%d. share_core_: %d. sem_enable_: %d. \
cpu_id_:%d\n", instance_id_, share_core_, sem_enable_, cpu_id_ );

   if ( sem_enable_ ) {
      sem_set_id_ = PacketProcessor::lookup_vsem();

      share_p_idx_ = instance_id_ - 1;
      share_np_idx_= ( share_p_idx_ + 1 ) % n_share_core_x;

      std::cout << "!!!!!!!!!!!!!!!!! sem_enable is 1 " \
                << "p_idx: " << share_p_idx_ << " np_idx: " \
                << share_np_idx_ << std::endl;
   } 
   else {
      std::cout << "!!!!!!!!!!!!!!!!! sem_enable is 0" << std::endl;
   }
   
   PacketProcessor::ConfigurePorts(pp_config, this);
}

inline void MacSwapper::Run() {
   rx_pkt_array_t rx_packets;
   register int16_t i = 0;
   const int16_t kNumPrefetch = 8;
   struct ether_hdr* eth_hdr = nullptr;
   uint16_t num_rx = 0;
   int res = 0;
   uint32_t counter = 0;
   while (true) {

      // Wait for my turn to use the CPU
      if ( sem_enable_ ) {
         res = PacketProcessor::wait_vsem( sem_set_id_, share_p_idx_ );
         if ( res < 0 )
            perror( "ERROR: wait_vsem()." );
      }

      num_rx = this->ingress_ports_[0]->RxBurst(rx_packets);
      for (i = 0; i < num_rx && i < kNumPrefetch; ++i)
         rte_prefetch0(rte_pktmbuf_mtod(rx_packets[i], void*));
      for (i = 0; i < num_rx - kNumPrefetch; ++i) {
         rte_prefetch0(rte_pktmbuf_mtod(rx_packets[i + kNumPrefetch], void*));
         eth_hdr = rte_pktmbuf_mtod(rx_packets[i], struct ether_hdr*);
         std::swap(eth_hdr->s_addr.addr_bytes, eth_hdr->d_addr.addr_bytes);
      }
      for ( ; i < num_rx; ++i) {
         eth_hdr = rte_pktmbuf_mtod(rx_packets[i], struct ether_hdr*);
         std::swap(eth_hdr->s_addr.addr_bytes, eth_hdr->d_addr.addr_bytes);

         // imitate processing
         int n = 5;
         int r = 0;
         for ( int i = 0; i < n; i++ ) {
            r = rand() % 30;
         }

      }
 
      this->egress_ports_[0]->TxBurst(rx_packets, num_rx);
      for (i=0; i < num_egress_ports_; i++){
         if (this->scale_bits->bits[this->instance_id_].test(i)){
            // TODO  Change port to smart port.
            this->scale_bits->bits[this->instance_id_].set(i, false);
         }
      }
    
      // Signal Semaphore and yield CPU
      if ( sem_enable_ ) {
         res = PacketProcessor::post_vsem( sem_set_id_, share_np_idx_ );
         if ( res < 0 )
            perror( "ERROR: post_vsem()." );
      }

      if ( share_core_ ) {
         if ( counter % 2 == 0 ) {
            res = sched_yield();
            if ( res == -1 ) {
               std::cerr << "sched_yield failed! Exitting." << std::endl;
               return;
            }
         }
      }
      counter++;
   }
}

inline void MacSwapper::FlushState() {}
inline void MacSwapper::RecoverState() {}
