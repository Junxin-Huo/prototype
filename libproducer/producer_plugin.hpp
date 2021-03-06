#pragma once
#include <boost/asio.hpp>
#include "chain_controller.hpp"
#include "types.hpp"
#include <set>
#include <string>
#include <functional>
#include <libethereum/BlockChain.h>
#include "chainbase.hpp"

namespace dev {
namespace eth {

class Client;


namespace block_production_condition {
	enum block_production_condition_enum
	{
		produced = 0,
		not_synced = 1,
		not_my_turn = 2,
		not_time_yet = 3,
		no_private_key = 4,
		low_participation = 5,
		lag = 6,
		consecutive = 7,
		exception_producing_block = 8
	};
}

class producer_plugin {
public:
	producer_plugin( dev::eth::BlockChain& bc);
	~producer_plugin() {
		//_chain.reset();
		//_db.reset();
	};

	void schedule_production_loop(int microsecond);


	block_production_condition::block_production_condition_enum should_produce();
	block_production_condition::block_production_condition_enum maybe_produce_block();

	chain::chain_controller& get_chain_controller() { return *_chain; }
	const fc::ecc::private_key& get_private_key(types::AccountName& address)const {
		if (_private_keys.find(address) != _private_keys.end())
			return _private_keys.at(address);
		else
			return fc::ecc::private_key();
	}

	chainbase::database& get_db() { return *_db; }

	void setClient(Client& c) { _client = &c; };
private:
	std::shared_ptr<boost::asio::io_service> _io_server;
	boost::asio::deadline_timer _timer;
	std::unique_ptr<chain::chain_controller> _chain;
	std::shared_ptr<chainbase::database> _db;
	Client* _client;

	bool _production_enabled = false;
	types::AccountNames _producers;
	std::map<types::AccountName, fc::ecc::private_key> _private_keys;
};

}
}
