#include "chain_controller.hpp"
#include "rand.hpp"
//#include "BlockChain.h"
#include <iostream>
//#include "../libevm/Vote.h"
#include "producer_objects.hpp"

using namespace dev;
using namespace dev::eth;
using namespace dev::eth::chain;
using namespace eos::chain;
using native::eos::ProducerVotesMultiIndex;
using native::eos::ProducerScheduleMultiIndex;


chain_controller::chain_controller(const dev::eth::BlockChain& bc, chainbase::database& db): _bc(bc),_db(db) {

	initialize_indexes();
	initialize_chain(bc);

	//starter.register_types(*this, _db);

	//// Behave as though we are applying a block during chain initialization (it's the genesis block!)
	//with_applying_block([&] {
	//	initialize_chain(starter);
	//});

	//spinup_db();
	//spinup_fork_db();

	//if (_block_log.read_head() && head_block_num() < _block_log.read_head()->block_num())
	//	replay();
}

chain_controller::~chain_controller() {
	_db.flush();
}

void chain_controller::initialize_indexes() {
	//_db.add_index<account_index>();
	//_db.add_index<permission_index>();
	//_db.add_index<permission_link_index>();
	//_db.add_index<action_permission_index>();
	//_db.add_index<key_value_index>();
	//_db.add_index<key128x128_value_index>();
	//_db.add_index<key64x64x64_value_index>();

	_db.add_index<global_property_multi_index>();
	_db.add_index<dynamic_global_property_multi_index>();
	_db.add_index<ProducerVotesMultiIndex>();
	_db.add_index<ProducerScheduleMultiIndex>();
	_db.add_index<producer_multi_index>();

	//_db.add_index<block_summary_multi_index>();
	//_db.add_index<transaction_multi_index>();
	//_db.add_index<generated_transaction_multi_index>();
}

void chain_controller::initialize_chain(const dev::eth::BlockChain& bc)
{
	try {
		if (!_db.find<global_property_object>()) 
		{
			_db.with_write_lock([&] {

				//auto initial_timestamp = fc::time_point_sec(bc.genesis().timestamp().convert_to<uint32_t>());
				/// TODO: ʹ�ô�����ʱ�����ʼ��
				auto initial_timestamp = fc::time_point_sec(1513580324);

				// Create global properties
				_db.create<global_property_object>([&](global_property_object& p) {
					//p.configuration = starter.get_chain_start_configuration();
					//p.active_producers = starter.get_chain_start_producers();
					
					p.active_producers = bc.chainParams().initialProducers;
				});
				_db.create<dynamic_global_property_object>([&](dynamic_global_property_object& p) {
					p.time = initial_timestamp;
					p.recent_slots_filled = uint64_t(-1);
				});

				// Create the singleton object, ProducerScheduleObject
				_db.create<native::eos::ProducerScheduleObject>([](const auto&) {});

				for (const auto& a : _db.get<global_property_object>().active_producers)
				{
					_db.create<producer_object>([&](producer_object& p) {
						p.owner = a;
					});
				}

				//// Initialize block summary index
				//for (int i = 0; i < 0x10000; i++)
				//	_db.create<block_summary_object>([&](block_summary_object&) {});

				//auto messages = starter.prepare_database(*this, _db);
				//std::for_each(messages.begin(), messages.end(), [&](const Message& m) {
				//	MessageOutput output;
				//	ProcessedTransaction trx; /// dummy tranaction required for scope validation
				//	std::sort(trx.scope.begin(), trx.scope.end());
				//	with_skip_flags(skip_scope_check | skip_transaction_signatures | skip_authority_check, [&]() {
				//		process_message(trx, m.code, m, output);
				//	});
				//});
			});
		}

		const auto& AllProducers = _db.get_index<ProducerVotesMultiIndex, native::eos::byVotes>();

		for (auto& p : AllProducers)
		{
			_all_votes.insert(std::pair<Address, uint64_t>(p.ownerName, p.getVotes()));
		}

	}
	catch (...) {
		cdebug << "initialize_chain error";
	}
}

fc::time_point_sec chain_controller::get_slot_time(uint32_t slot_num)const
{
	if (slot_num == 0)
		return fc::time_point_sec();

	auto interval = block_interval();
	const dynamic_global_property_object& dpo = get_dynamic_global_properties();

	if (head_block_num() == 0)
	{
		// n.b. first block is at genesis_time plus one block interval
		fc::time_point_sec genesis_time = dpo.time;
		return genesis_time + slot_num * interval;
	}

	int64_t head_block_abs_slot = head_block_time().sec_since_epoch() / interval;
	fc::time_point_sec head_slot_time(head_block_abs_slot * interval);

	return head_slot_time + (slot_num * interval);
}

const global_property_object& chain_controller::get_global_properties()const {
	return _db.get<global_property_object>();
}

const dynamic_global_property_object&chain_controller::get_dynamic_global_properties() const {
	return _db.get<dynamic_global_property_object>();
}

fc::time_point_sec chain_controller::head_block_time()const {
	return get_dynamic_global_properties().time;
}

uint32_t chain_controller::head_block_num()const {
	return get_dynamic_global_properties().head_block_number;
}

uint32_t chain_controller::get_slot_at_time(fc::time_point_sec when)const
{
	fc::time_point_sec first_slot_time = get_slot_time(1);
	if (when < first_slot_time)
		return 0;
	return (when - first_slot_time).to_seconds() / block_interval() + 1;
}

types::AccountName chain_controller::get_scheduled_producer(uint32_t slot_num)const
{
	const dynamic_global_property_object& dpo = get_dynamic_global_properties();
	uint64_t current_aslot = dpo.current_absolute_slot + slot_num;
	const auto& gpo = _db.get<global_property_object>();
	return gpo.active_producers[current_aslot % gpo.active_producers.size()];
}

const producer_object& chain_controller::get_producer(const types::AccountName& ownerName) const {
	return _db.get<producer_object, by_owner>(ownerName);
}

ProducerRound chain_controller::calculate_next_round(const BlockHeader& next_block) {
	auto schedule = native::eos::ProducerScheduleObject::get(_db).calculateNextRound(_db);
	//auto changes = get_global_properties().active_producers - schedule;
	//EOS_ASSERT(boost::range::equal(next_block.producer_changes, changes), block_validate_exception,
	//	"Unexpected round changes in new block header",
	//	("expected changes", changes)("block changes", next_block.producer_changes));

	utilities::rand::random rng(next_block.timestamp().convert_to<uint64_t>());
	rng.shuffle(schedule);
	return schedule;
}


void chain_controller::update_global_properties(const BlockHeader& b) {
	// If we're at the end of a round, update the BlockchainConfiguration, producer schedule
	// and "producers" special account authority
	if (b.number().convert_to<uint32_t>() % config::BlocksPerRound == 0) {
		try {
			auto schedule = calculate_next_round(b);
			//auto config = _admin->get_blockchain_configuration(_db, schedule);

			const auto& gpo = get_global_properties();
			_db.modify(gpo, [schedule = std::move(schedule)](global_property_object& gpo) {
				gpo.active_producers = std::move(schedule);
				//gpo.configuration = std::move(config);
			});
		}
		catch (NoEnoughProducers const& e) {
			cdebug << e.what();
			return;
		}
		catch (...) {
			cdebug << "calculate_next_round error";
			return;
		}

		//auto active_producers_authority = types::Authority(config::ProducersAuthorityThreshold, {}, {});
		//for (auto& name : gpo.active_producers) {
		//	active_producers_authority.accounts.push_back({ { name, config::ActiveName }, 1 });
		//}

		//auto& po = _db.get<permission_object, by_owner>(boost::make_tuple(config::ProducersAccountName, config::ActiveName));
		//_db.modify(po, [active_producers_authority](permission_object& po) {
		//	po.auth = active_producers_authority;
		//});
	}
}

//std::map<Address, VoteBace> chain_controller::get_votes(h256 const& _hash) const
//{
//	if (_stateDB == nullptr)
//		return std::map<Address, VoteBace>();
//
//	Block block(_bc, *_stateDB);
//	block.populateFromChain(_bc, _hash == h256() ? _bc.currentHash() : _hash);
//
//
//	std::unordered_map<u256, u256> voteMap = block.storage(Address("0000000000000000000000000000000000000005"));
//	std::unordered_map<u256, u256> voteMapChange;
//	std::map<Address, VoteBace> returnMap;
//	dev::bytes bytes0(12, '\0');
//	for (auto iterator = voteMap.begin(); iterator != voteMap.end(); iterator++)
//	{
//		dev::u256 iteratorFirst = iterator->first;
//		dev::bytes iteratorBytes = ((dev::h256)iteratorFirst).asBytes();
//		dev::bytes iteratorAddress(iteratorBytes.begin(), iteratorBytes.begin() + sizeof(dev::Address));
//		dev::bytes iteratorExpand(iteratorBytes.begin() + sizeof(dev::Address), iteratorBytes.end());
//
//		if (iteratorExpand == bytes0)
//		{
//			Vote reset(voteMap, voteMapChange, dev::Address(iteratorAddress));
//			reset.load();
//			VoteBace vb;
//			vb.m_address = dev::Address(iteratorAddress);
//			vb.m_assignNumber = reset.getAssignNumber();
//			vb.m_isCandidate = reset.getIsCandidate();
//			vb.m_isVoted = reset.getIsVoted();
//			vb.m_receivedVoteNumber = reset.getReceivedVoteNumber();
//			vb.m_unAssignNumber = reset.getUnAssignNumber();
//			vb.m_votedNumber = reset.getVotedNumber();
//			vb.m_voteTo = reset.getVoteTo();
//			returnMap[dev::Address(iteratorAddress)] = vb;
//		}
//	}
//	return returnMap;
//}

using native::eos::ProducerScheduleObject;
using native::eos::ProducerVotesObject;

void chain_controller::update_pvomi_perblock()
{
	//std::map<Address, VoteBace> AllProducers = get_votes();
	//std::unordered_set<Address> InactiveProducers;

	//// check every producer in AllProducers
	//InactiveProducers.clear();
	//for (const auto& p : AllProducers)
	//{
	//	auto it = _all_votes.find(p.first);
	//	if (it != _all_votes.end())
	//	{
	//		dev::types::Int64 deltaVotes = p.second.m_votedNumber - it->second;
	//		if (deltaVotes == 0)
	//			continue;

	//		auto raceTime = ProducerScheduleObject::get(_db).currentRaceTime;
	//		_db.modify<ProducerVotesObject>(_db.get<ProducerVotesObject, native::eos::byOwnerName>(p.first), [&](ProducerVotesObject& pvo) {
	//			pvo.updateVotes(deltaVotes, raceTime);
	//		});
	//		it->second = p.second.m_votedNumber;
	//	}
	//	else
	//	{
	//		auto raceTime = ProducerScheduleObject::get(_db).currentRaceTime;
	//		_db.create<ProducerVotesObject>([&](ProducerVotesObject& pvo) {
	//			pvo.ownerName = p.first;
	//			pvo.startNewRaceLap(raceTime);
	//		});

	//		_db.create<producer_object>([&](producer_object& po) {
	//			po.owner = p.first;
	//			//po.signing_key = update.key;
	//			//po.configuration = update.configuration;
	//		});
	//		
	//		_all_votes.insert(std::make_pair(p.first, p.second.m_votedNumber));
	//	}
	//}

	//// remove all inactive producers
	//for (const auto& p : _all_votes)
	//{
	//	if (AllProducers.find(p.first) == AllProducers.end())
	//	{
	//		InactiveProducers.insert(p.first);
	//	}
	//}

	//for (auto& p : InactiveProducers)
	//{
	//	_db.remove<ProducerVotesObject>(_db.get<ProducerVotesObject, native::eos::byOwnerName>(p));
	//	_db.remove<producer_object>(_db.get<producer_object, eos::chain::by_owner>(p));
	//	_all_votes.erase(p);
	//}

	//ctrace << "ProducerVotesMultiIndex: ";
	//for ( auto& a : _db.get_index<ProducerVotesMultiIndex, native::eos::byProjectedRaceFinishTime>())
	//{
	//	ctrace << "name: " << a.ownerName;
	//	ctrace << "votes: " << a.getVotes();
	//	ctrace << "finish time: " << a.projectedRaceFinishTime();
	//}
}

void chain_controller::update_global_dynamic_data(const BlockHeader& b) {
	const dynamic_global_property_object& _dgp = _db.get<dynamic_global_property_object>();

	uint32_t missed_blocks = head_block_num() == 0 ? 1 : get_slot_at_time(fc::time_point_sec(b.timestamp().convert_to<uint32_t>()));
	assert(missed_blocks != 0);
	missed_blocks--;

	//   if (missed_blocks)
	//      wlog("Blockchain continuing after gap of ${b} missed blocks", ("b", missed_blocks));

	for (uint32_t i = 0; i < missed_blocks; ++i) {
		const auto& producer_missed = get_producer(get_scheduled_producer(i + 1));
		if (producer_missed.owner != b.producer()) {
			/*
			const auto& producer_account = producer_missed.producer_account(*this);
			if( (fc::time_point::now() - b.timestamp) < fc::seconds(30) )
			wlog( "Producer ${name} missed block ${n} around ${t}", ("name",producer_account.name)("n",b.block_num())("t",b.timestamp) );
			*/

			_db.modify(producer_missed, [&](producer_object& w) {
				w.total_missed++;
			});
		}
	}

	// dynamic global properties updating
	_db.modify(_dgp, [&](dynamic_global_property_object& dgp) {
		dgp.head_block_number = b.number().convert_to<uint32_t>();
		dgp.head_block_id = b.hash();
		dgp.time = fc::time_point_sec(b.timestamp().convert_to<uint32_t>());
		dgp.current_producer = b.producer();
		dgp.current_absolute_slot += missed_blocks + 1;

		// If we've missed more blocks than the bitmap stores, skip calculations and simply reset the bitmap
		if (missed_blocks < sizeof(dgp.recent_slots_filled) * 8) {
			dgp.recent_slots_filled <<= 1;
			dgp.recent_slots_filled += 1;
			dgp.recent_slots_filled <<= missed_blocks;
		}
		else
			dgp.recent_slots_filled = 0;
	});

}

void chain_controller::update_signing_producer(const producer_object& signing_producer, const BlockHeader& new_block)
{
	const dynamic_global_property_object& dpo = get_dynamic_global_properties();
	uint64_t new_block_aslot = dpo.current_absolute_slot + get_slot_at_time(fc::time_point_sec(new_block.timestamp().convert_to<uint32_t>()));

	_db.modify(signing_producer, [&](producer_object& _wit)
	{
		_wit.last_aslot = new_block_aslot;
		_wit.last_confirmed_block_num = new_block.number().convert_to<uint32_t>();
	});
}

void chain_controller::update_last_irreversible_block()
{
	const global_property_object& gpo = get_global_properties();
	const dynamic_global_property_object& dpo = get_dynamic_global_properties();

	std::vector<const producer_object*> producer_objs;
	producer_objs.reserve(gpo.active_producers.size());
	std::transform(gpo.active_producers.begin(), gpo.active_producers.end(), std::back_inserter(producer_objs),
		[this](const AccountName& owner) { return &get_producer(owner); });

	static_assert(config::IrreversibleThresholdPercent > 0, "irreversible threshold must be nonzero");

	size_t offset = dev::eth::ETH_PERCENT(producer_objs.size(), config::Percent100 - config::IrreversibleThresholdPercent);
	std::nth_element(producer_objs.begin(), producer_objs.begin() + offset, producer_objs.end(),
		[](const producer_object* a, const producer_object* b) {
		return a->last_confirmed_block_num < b->last_confirmed_block_num;
	});

	uint32_t new_last_irreversible_block_num = producer_objs[offset]->last_confirmed_block_num;

	if (new_last_irreversible_block_num > dpo.last_irreversible_block_num) {
		_db.modify(dpo, [&](dynamic_global_property_object& _dpo) {
			_dpo.last_irreversible_block_num = new_last_irreversible_block_num;
		});
	}

	// Trim fork_database and undo histories
	//_fork_db.set_max_size(head_block_num() - new_last_irreversible_block_num + 1);
	_db.commit(new_last_irreversible_block_num);
}

void chain_controller::databaseReversion(uint32_t _firstvalid)
{
	while (head_block_num() != _firstvalid)
	{
		_db.undo();
	}
}

void chain_controller::push_block(const BlockHeader& b)
{
	try {
		auto session = _db.start_undo_session(true);
		apply_block(b);
		session.push();
	}
	catch (...)
	{
		cwarn << "apply block error: " << b.number();
		throw;
	}

}

void chain_controller::apply_block(const BlockHeader& b)
{
	const producer_object& signing_producer = validate_block_header(b);

	update_pvomi_perblock();
	update_global_dynamic_data(b);
	update_global_properties(b);
	update_signing_producer(signing_producer, b);
	update_last_irreversible_block();
}

const producer_object& chain_controller::validate_block_header(const BlockHeader& bh)const
{
	if (!bh.verifySign())
		BOOST_THROW_EXCEPTION(InvalidSignature());

	// verify sender is at this slot
	uint32_t slot = get_slot_at_time(fc::time_point_sec(bh.timestamp().convert_to<uint32_t>()));
	types::AccountName scheduled_producer = get_scheduled_producer(slot);

	const types::AccountName& producer = bh.producer();

	ctrace << "scheduled_producer: " << scheduled_producer;
	ctrace << "block producer: " << producer;
	if (scheduled_producer != producer)
		BOOST_THROW_EXCEPTION(InvalidProducer());

	return get_producer(scheduled_producer);
}