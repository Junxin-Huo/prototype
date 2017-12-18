#include "producer_plugin.hpp"
#include <fc/time.hpp>
////#include "fc/variant.hpp"
//#include <fc/variant_object.hpp>
////#include <libdevcore/Log.h>
//#include <iostream>
#include <libethereum/Client.h>

using namespace dev::eth;

producer_plugin::producer_plugin(const dev::eth::BlockChain& bc, dev::eth::Client& client):
	_client(client),
	_io_server(std::make_shared<boost::asio::io_service>()),
	_timer(*_io_server)
{
	auto dbPath = bc.dbPath().string() + "/" + toHex(bc.genesisHash().ref().cropped(0, 4));
	_db = std::make_shared<chainbase::database>(dbPath, chainbase::database::read_write, 10 * 1024 * 1024);

	_chain = std::make_unique<chain::chain_controller>(bc, *_db);

	for (auto k : bc.chainParams().privateKeys)
	{
		_private_keys.insert(k);
	}
	_production_enabled = bc.chainParams().enableStaleProduction;
	_producers = bc.chainParams().accountNames;
}

void producer_plugin::schedule_production_loop() 
{
	//Schedule for the next second's tick regardless of chain state
	// If we would wait less than 50ms, wait for the whole second.
	fc::time_point now = fc::time_point::now();
	int64_t time_to_next_second = 1000000 - (now.time_since_epoch().count() % 1000000);
	if (time_to_next_second < 50000)      // we must sleep for at least 50ms
		time_to_next_second += 1000000;

	_timer.expires_from_now(boost::posix_time::microseconds(time_to_next_second));
	_timer.wait();
	//_timer.async_wait(boost::bind(&producer_plugin::block_production_loop, this));
}

block_production_condition::block_production_condition_enum producer_plugin::should_produce()
{
	block_production_condition::block_production_condition_enum result;

	try
	{
		result = maybe_produce_block();
	}
	catch (...)
	{
		std::cerr << "Got exception while generating block: xx" << std::endl;
		result = block_production_condition::exception_producing_block;
	}

	switch (result)
	{

	case block_production_condition::produced: {
		ctrace << "Should produce new block";
		break;
	}
	case block_production_condition::not_synced:
		ctrace << "Not producing block because production is disabled until we receive a recent block ";
		break;
	case block_production_condition::not_my_turn:
		ctrace << "Not producing block because it isn't my turn";
		break;
	case block_production_condition::not_time_yet:
		ctrace << "Not producing block because slot has not yet arrived";
		break;
	case block_production_condition::no_private_key:
		ctrace << "Not producing block because I don't have the private key for ${scheduled_key}";
		break;
	case block_production_condition::low_participation:
		cnote << "Not producing block because node appears to be on a minority fork with only ${pct}% producer participation";
		break;
	case block_production_condition::lag:
		cnote << "Not producing block because node didn't wake up within 500ms of the slot time.";
		break;
	case block_production_condition::consecutive:
		cnote << "Not producing block because the last block was generated by the same producer.\nThis node is probably disconnected from the network so block production has been disabled.\nDisable this check with --allow-consecutive option.";
		break;
	case block_production_condition::exception_producing_block:
		cnote << "exception prodcing block";
		break;
	}

	return result;
}

block_production_condition::block_production_condition_enum producer_plugin::maybe_produce_block()
{
	chain::chain_controller& chain = *_chain;

	fc::time_point now_fine = fc::time_point::now();
	fc::time_point_sec now = now_fine + fc::microseconds(500000);

	// If the next block production opportunity is in the present or future, we're synced.
	if (!_production_enabled)
	{
		cdebug << chain.get_slot_time(1).sec_since_epoch();
		cdebug << now.sec_since_epoch();

		if (chain.get_slot_time(1) >= now)
			_production_enabled = true;
		else
			return block_production_condition::not_synced;
	}

	// is anyone scheduled to produce now or one second in the future?
	uint32_t slot = chain.get_slot_at_time(now);
	ctrace << "slot num: " << slot;
	if (slot == 0)
	{
		return block_production_condition::not_time_yet;
	}

	//
	// this assert should not fail, because now <= db.head_block_time()
	// should have resulted in slot == 0.
	//
	// if this assert triggers, there is a serious bug in get_slot_at_time()
	// which would result in allowing a later block to have a timestamp
	// less than or equal to the previous block
	//
	assert(now > chain.head_block_time());

	types::AccountName scheduled_producer = chain.get_scheduled_producer(slot);
	ctrace << "scheduled_producer: " << scheduled_producer;
	// we must control the producer scheduled to produce the next block.
	if (_producers.find(scheduled_producer) == _producers.end())
	{
		return block_production_condition::not_my_turn;
	}

	fc::time_point_sec scheduled_time = chain.get_slot_time(slot);
	//chain::public_key_type scheduled_key = chain.get_producer(scheduled_producer).signing_key;
	auto private_key_itr = _private_keys.find(scheduled_producer);

	if (private_key_itr == _private_keys.end())
	{
		return block_production_condition::no_private_key;
	}

	/// TODO 小于设置的参与率不能出块
	//uint32_t prate = chain.producer_participation_rate();
	//if (prate < _required_producer_participation)
	//{
	//	return block_production_condition::low_participation;
	//}

	/// TODO 落后出块时间500ms不能出快
	if (llabs((scheduled_time - now).count()) > fc::milliseconds(500).count())
	{
		//capture("scheduled_time", scheduled_time)("now", now);
		return block_production_condition::lag;
	}

	/// 出块
	try
	{
		_client.generate_block(scheduled_time, scheduled_producer, private_key_itr->second);
	}
	catch(...)
	{
		return block_production_condition::exception_producing_block;
	}
	//generate_block(scheduled_time, scheduled_producer, private_key_itr->second, 0);
	//auto block = chain.generate_block(
	//	scheduled_time,
	//	scheduled_producer,
	//	private_key_itr->second
	//);

	//uint32_t count = 0;
	//for (const auto& cycle : block.cycles) {
	//	for (const auto& thread : cycle) {
	//		count += thread.generated_input.size();
	//		count += thread.user_input.size();
	//	}
	//}

	//capture("n", block.block_num())("t", block.timestamp)("c", now)("count", count);

	//app().get_plugin<net_plugin>().broadcast_block(block);

	//std::cout << (block.timestamp.sec_since_epoch()) << std::endl;

	//ilog("producer: ");
	//std::cout << block.producer << std::endl;
	//ilog("private key: ");
	//std::cout << std::hex << block.block_signing_private_key << std::endl;

	return block_production_condition::produced;
}
