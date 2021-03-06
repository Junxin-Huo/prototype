#pragma once
//#include <fc/uint128.hpp>
//#include <fc/array.hpp>
//
//#include <eos/chain/types.hpp>
//#include <eos/chain/BlockchainConfiguration.hpp>
//
//#include <chainbase/chainbase.hpp>


#include "chainbase.hpp"
#include "types.hpp"
#include "fc/time.hpp"

#include "multi_index_includes.hpp"
#include <vector>
#include "version.hpp"

using dev::Address;
using dev::types::AccountName;
using dev::types::block_id_type;
using dev::eth::config::TotalProducersPerRound;
using dev::eth::chain::hardfork_version;

namespace eos { namespace chain {


	class process_hardfork_object : public chainbase::object<process_hardfork_object_type, process_hardfork_object>
	{ 
		OBJECT_CTOR(process_hardfork_object)

		id_type id;
		fc::time_point_sec hardfork_timepoint;
	};

   /**
    * @class global_property_object
    * @brief Maintains global state information (committee_member list, current fees)
    * @ingroup object
    * @ingroup implementation
    *
    * This is an implementation detail. The values here are set by committee_members to tune the blockchain parameters.
    */
   class global_property_object : public chainbase::object<global_property_object_type, global_property_object>
   {
      OBJECT_CTOR(global_property_object)

      id_type id;  
	  std::array<AccountName, TotalProducersPerRound> active_producers;


	  //==============Hardfork����=======================
	   
	  int							last_hardfork = 0;
	  hardfork_version				current_hardfork_version;
	  hardfork_version				next_hardfork;
	  fc::time_point_sec			next_hardfork_time;
   };



   /**
    * @class dynamic_global_property_object
    * @brief Maintains global state information (committee_member list, current fees)
    * @ingroup object
    * @ingroup implementation
    *
    * This is an implementation detail. The values here are calculated during normal chain operations and reflect the
    * current values of global blockchain properties.
    */
   class dynamic_global_property_object : public chainbase::object<dynamic_global_property_object_type, dynamic_global_property_object>
   {
        OBJECT_CTOR(dynamic_global_property_object)

        id_type           id;
        uint32_t          head_block_number = 0;
        block_id_type     head_block_id;
        fc::time_point_sec    time;
        AccountName       current_producer;
        uint32_t          accounts_registered_this_interval = 0;
        
        /**
         * The current absolute slot number.  Equal to the total
         * number of slots since genesis.  Also equal to the total
         * number of missed slots plus head_block_number.
         */
        uint64_t          current_absolute_slot = 0;
        
        /**
         * Bitmap used to compute producer participation. Stores
         * a high bit for each generated block, a low bit for
         * each missed block. Least significant bit is most
         * recent block.
         *
         * NOTE: This bitmap always excludes the head block,
         * which, by definition, exists. The least significant
         * bit corresponds to the block with number
         * head_block_num()-1
         *
         * e.g. if the least significant 5 bits were 10011, it
         * would indicate that the last two blocks prior to the
         * head block were produced, the two before them were
         * missed, and the one before that was produced.
         */
        uint64_t recent_slots_filled;
        
        uint32_t last_irreversible_block_num = 0;

		//===============POW ���=====================

		/**
		*  The total POW accumulated, aka the sum of num_pow_witness at the time new POW is added
		*/
		uint64_t total_pow = 0;

		/**
		* The current count of how many pending POW witnesses there are, determines the difficulty
		* of doing pow
		*/
		uint32_t num_pow_witnesses = 0;
   };

   struct by_time;
   using process_hardfork_multi_index = chainbase::shared_multi_index_container<
	   process_hardfork_object,
	   indexed_by<
		ordered_unique<tag<by_id>, member<process_hardfork_object, process_hardfork_object::id_type, &process_hardfork_object::id>>,
		ordered_unique<tag<by_time>, member<process_hardfork_object,fc::time_point_sec, &process_hardfork_object::hardfork_timepoint>>
	   >
   >;

   using global_property_multi_index = chainbase::shared_multi_index_container<
      global_property_object,
      indexed_by<
         ordered_unique<tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(global_property_object, global_property_object::id_type, id)
         >
      >
   >;

   using dynamic_global_property_multi_index = chainbase::shared_multi_index_container<
      dynamic_global_property_object,
      indexed_by<
         ordered_unique<tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(dynamic_global_property_object, dynamic_global_property_object::id_type, id)
         >
      >
   >;

}}

CHAINBASE_SET_INDEX_TYPE(eos::chain::process_hardfork_object, eos::chain::process_hardfork_multi_index)
CHAINBASE_SET_INDEX_TYPE(eos::chain::global_property_object, eos::chain::global_property_multi_index)
CHAINBASE_SET_INDEX_TYPE(eos::chain::dynamic_global_property_object,
                         eos::chain::dynamic_global_property_multi_index)



FC_REFLECT(eos::chain::process_hardfork_object,
			(hardfork_timepoint)
		  )

FC_REFLECT(eos::chain::dynamic_global_property_object,
           (head_block_number)
           (head_block_id)
           (time)
           (current_producer)
           (accounts_registered_this_interval)
           (current_absolute_slot)
           (recent_slots_filled)
           (last_irreversible_block_num)
		   (total_pow)
		   (num_pow_witnesses)
          )

FC_REFLECT(eos::chain::global_property_object,
           (active_producers) 
		   (last_hardfork)
		   (current_hardfork_version)
		   (next_hardfork)
		   (next_hardfork_time)
          )
