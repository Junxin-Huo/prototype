#include "BlockChainSyncState.h"
#include "BlockChainSync.h"

#include <chrono>
#include <libdevcore/Common.h>
#include <libdevcore/TrieHash.h>
#include <libp2p/Host.h>
#include <libp2p/Session.h>
#include <libethcore/Exceptions.h>
#include "BlockChain.h"
#include "BlockQueue.h"
#include "EthereumPeer.h"
#include "EthereumHost.h"
#include "libdevcore/Log.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace p2p;


unsigned const c_maxPeerUknownNewBlocks = 1024; /// Max number of unknown new blocks peer can give us
unsigned const c_maxRequestHeaders = 1024;
unsigned const c_maxRequestBodies = 1024;
unsigned const c_blockQueueGap = 1000;

namespace  // Helper functions.
{

	template<typename T> bool haveItem(std::map<unsigned, T>& _container, unsigned _number)
	{
		if (_container.empty())
			return false;
		auto lower = _container.lower_bound(_number);
		if (lower != _container.end() && lower->first == _number)
			return true;
		if (lower == _container.begin())
			return false;
		--lower;
		return lower->first <= _number && (lower->first + lower->second.size()) > _number;
	}

	template<typename T> T const* findItem(std::map<unsigned, std::vector<T>>& _container, unsigned _number)
	{
		if (_container.empty())
			return nullptr;
		auto lower = _container.lower_bound(_number);
		if (lower != _container.end() && lower->first == _number)
			return &(*lower->second.begin());
		if (lower == _container.begin())
			return nullptr;
		--lower;
		if (lower->first <= _number && (lower->first + lower->second.size()) > _number)
			return &lower->second.at(_number - lower->first);
		return nullptr;
	}

	template<typename T> void removeItem(std::map<unsigned, std::vector<T>>& _container, unsigned _number)
	{
		if (_container.empty())
			return;
		auto lower = _container.lower_bound(_number);
		if (lower != _container.end() && lower->first == _number)
		{
			_container.erase(lower);
			return;
		}
		if (lower == _container.begin())
			return;
		--lower;
		if (lower->first <= _number && (lower->first + lower->second.size()) > _number)
			lower->second.erase(lower->second.begin() + (_number - lower->first), lower->second.end());
	}

	template<typename T> void removeAllStartingWith(std::map<unsigned, std::vector<T>>& _container, unsigned _number)
	{
		if (_container.empty())
			return;
		auto lower = _container.lower_bound(_number);
		if (lower != _container.end() && lower->first == _number)
		{
			_container.erase(lower, _container.end());
			return;
		}
		if (lower == _container.begin())
		{
			_container.clear();
			return;
		}
		--lower;
		if (lower->first <= _number && (lower->first + lower->second.size()) > _number)
			lower->second.erase(lower->second.begin() + (_number - lower->first), lower->second.end());
		_container.erase(++lower, _container.end());
	}

	template<typename T> void mergeInto(std::map<unsigned, std::vector<T>>& _container, unsigned _number, T&& _data)
	{
		assert(!haveItem(_container, _number));
		auto lower = _container.lower_bound(_number);
		if (!_container.empty() && lower != _container.begin())
			--lower;
		if (lower != _container.end() && (lower->first + lower->second.size() == _number))
		{
			// extend existing chunk
			lower->second.emplace_back(_data);

			auto next = lower;
			++next;
			if (next != _container.end() && (lower->first + lower->second.size() == next->first))
			{
				// merge with the next chunk
				std::move(next->second.begin(), next->second.end(), std::back_inserter(lower->second));
				_container.erase(next);
			}

		}
		else
		{
			// insert a new chunk
			auto inserted = _container.insert(lower, std::make_pair(_number, std::vector<T> { _data }));
			auto next = inserted;
			++next;
			if (next != _container.end() && next->first == _number + 1)
			{
				std::move(next->second.begin(), next->second.end(), std::back_inserter(inserted->second));
				_container.erase(next);
			}
		}
	}

}  // Anonymous namespace -- helper functions.

namespace dev
{ 
	namespace eth
	{ 
		BlockChainSyncState::BlockChainSyncState(BlockChainSync& _sync):m_sync(_sync){}
		 
		void IdleSyncState::onPeerStatus(std::shared_ptr<EthereumPeer> _peer)
		{
			updateTimeout();

			if (peerLegalCheck(_peer))
			{
				if (_peer->getLastIrrBlock() < m_sync.m_lastIrreversibleBlock)
				{
					cwarn << "peer:" << _peer->id() << "|" << _peer->getLastIrrBlock() << " < " << "m_lastIrreversibleBlock = " << m_sync.m_lastIrreversibleBlock;
					return;
				}

				if (host().bq().knownCount() > 10)
				{
					return;
				}

				auto status = bq().blockStatus(_peer->m_latestHash);
				if (status == QueueStatus::Unknown)
				{//��peer��laststHashΪδ֪ʱ����ȡ����request��ͷ  


					//����ǰ��������Ϊ������ʼ��
					m_sync.m_syncStartBlock = m_sync.m_lastImportedBlock;
					m_sync.m_syncStartBlockHash = m_sync.m_lastImportedBlockHash;
					m_sync.m_syncLastIrrBlock = m_sync.m_lastIrreversibleBlock;

					m_sync.m_expectBlockHashForFindingCommon = _peer->m_latestHash;
					m_sync.m_expectBlockForFindingCommon = 0;

					//�������ж�ͬ��
					switchState(SyncState::FindingCommonBlock);

					requestPeerLatestBlockHeader(_peer); 

				}
			}
		}

		 

		void IdleSyncState::onPeerNewBlock(std::shared_ptr<EthereumPeer> _peer, RLP const& _r)
		{ 
			updateTimeout();

			if (_r.itemCount() != 3)
			{
				_peer->disable("NewBlock without 2 data fields.");
				return;
			}

			BlockHeader info(_r[0][0].data(), HeaderData);
			auto h = info.hash();


			cwarn << "onPeerNewBlock ==>" << h;

			//�����ݰ��л�ȡ��peer�Ĳ�����ת���
			uint32_t lastIrrBlock = _r[1].toInt<u256>().convert_to<uint32_t>();
			h256	lastIrrBlockHash = _r[2].toHash<h256>();

			cwarn << "LastIrr = " << lastIrrBlock;

			//����peer���²�����ת���
			_peer->setLastIrrBlock(lastIrrBlock);
			_peer->setLastIrrBlockHash(lastIrrBlockHash);

			//����Peer���µ�Hash
			_peer->m_latestHash = h;

			if (lastIrrBlock < m_sync.m_lastIrreversibleBlock)
			{//�ܾ����ղ�����תС�ڵ�ǰ������ת��Ŀͻ���

				cwarn << _peer->id() << "|" << lastIrrBlock << " < " << "m_lastIrreversibleBlock = " << m_sync.m_lastIrreversibleBlock << "ignore new block!!!!";
				_peer->addRating(-10000);
				return;
			}

			//ֻ����BlockQueueδ֪�Ŀ�
			auto status = host().bq().blockStatus(h);
			if (status != QueueStatus::Unknown)
			{
				return;
			}

			_peer->tryInsertPeerKnownBlockList(h); 

			unsigned blockNumber = static_cast<unsigned>(info.number());
			if (blockNumber > (m_sync.m_lastImportedBlock + 1))
			{
				cwarn << "Received unknown new block";

				if (host().bq().knownCount() < 10)
				{ 
					//����ǰ��������Ϊ������ʼ��
					m_sync.m_syncStartBlock = m_sync.m_lastImportedBlock;
					m_sync.m_syncStartBlockHash = m_sync.m_lastImportedBlockHash;
					m_sync.m_syncLastIrrBlock = m_sync.m_lastIrreversibleBlock;

					m_sync.m_expectBlockHashForFindingCommon = _peer->m_latestHash;
					m_sync.m_expectBlockForFindingCommon = 0;
					requestPeerLatestBlockHeader(_peer);

					//���Բ���CommonBlock
					switchState(SyncState::FindingCommonBlock);
				}
				return;
			}
			else if (
				blockNumber < m_sync.m_lastImportedBlock &&
				lastIrrBlock > m_sync.m_lastIrreversibleBlock
				)
			{//��ǰ��������������
				cwarn << "blockNumber < m_lastImportedBlock" << blockNumber << " < " << m_sync.m_lastImportedBlock;
				cwarn << "lastIrrBlock > m_lastIrreversibleBlock" << lastIrrBlock << " > " << m_sync.m_lastIrreversibleBlock;
				cwarn << "back2LastIrrBlockAndResync";
				
				m_sync.m_syncStartBlock = m_sync.m_lastIrreversibleBlock;
				m_sync.m_syncStartBlockHash = host().chain().numberHash(m_sync.m_syncStartBlock);
				m_sync.m_syncLastIrrBlock = m_sync.m_lastIrreversibleBlock;

				m_sync.m_expectBlockHashForFindingCommon = _peer->m_latestHash;
				m_sync.m_expectBlockForFindingCommon = 0;
				requestPeerLatestBlockHeader(_peer);

				//��������������������
				m_sync.m_lockBlockGen = true;
				cwarn << "Block Gen Locked!!!!!";
				//���Բ���CommonBlock
				switchState(SyncState::FindingCommonBlock);
				return;
			}


			switch (host().bq().import(_r[0].data()))
			{
				case ImportResult::Success:
					_peer->addRating(100);  

					if (blockNumber > m_sync.m_lastImportedBlock)
					{
						m_sync.m_lastImportedBlock = max(m_sync.m_lastImportedBlock, blockNumber);
						m_sync.m_lastImportedBlockHash = h;
					}

					//�ӵ������Ŀ飬���򵥵ļ�����ֱ�ӹ㲥��ȥ
					host().pushDeliverBlock(h, _r[0].data().toBytes(), lastIrrBlock, lastIrrBlockHash);
					break;
				case ImportResult::FutureTimeKnown:
				 
					//�ӵ������Ŀ飬���򵥵ļ�����ֱ�ӹ㲥��ȥ
					host().pushDeliverBlock(h, _r[0].data().toBytes(), lastIrrBlock, lastIrrBlockHash);
					break;

				case ImportResult::Malformed:
				case ImportResult::BadChain: 
					_peer->disable("Malformed block received.");
					return;

				case ImportResult::AlreadyInChain:
				case ImportResult::AlreadyKnown:
					break;

				case ImportResult::FutureTimeUnknown:
				case ImportResult::UnknownParent:
				{
					_peer->m_unknownNewBlocks++;
					if (_peer->m_unknownNewBlocks > c_maxPeerUknownNewBlocks)
					{
						_peer->disable("Too many uknown new blocks");
						resetAllSyncData();
					}
					//����ǰ��������Ϊ������ʼ��
					m_sync.m_syncStartBlock = m_sync.m_lastImportedBlock;
					m_sync.m_syncStartBlockHash = m_sync.m_lastImportedBlockHash;
					m_sync.m_syncLastIrrBlock = m_sync.m_lastIrreversibleBlock;

					m_sync.m_expectBlockHashForFindingCommon = _peer->m_latestHash;
					m_sync.m_expectBlockForFindingCommon = 0;
					requestPeerLatestBlockHeader(_peer);

					//���Բ���CommonBlock
					switchState(SyncState::FindingCommonBlock);
					break;
				}
				case ImportResult::Irreversible: //������δ֪�Ĳ�����ת�飬˵��ĳ�ͻ����뵱ǰ�ͻ���������ƫ��
					cwarn << "Unknown irreversible block founded!!! ignore and restart sync!";
					_peer->addRating(-10000);
					_peer->disable("Unknown irreversible block founded!!!");
					resetAllSyncData(); 
					break;
				default:;
			}
		}

		void IdleSyncState::onPeerNewHashes(std::shared_ptr<EthereumPeer> _peer, std::vector<std::pair<h256, u256>> const& _hashes)
		{
			updateTimeout();

			if (_peer->isConversing())
			{
				clog(NetMessageDetail) << "Ignoring new hashes since we're already downloading.";
				return;
			}
			clog(NetMessageDetail) << "Not syncing and new block hash discovered: syncing.";
			unsigned knowns = 0;
			unsigned unknowns = 0;
			unsigned maxHeight = 0;

			for (auto const& p : _hashes)
			{
				h256 const& h = p.first;
				_peer->addRating(1);
				_peer->tryInsertPeerKnownBlockList(h);
				auto status = host().bq().blockStatus(h);
				if (status == QueueStatus::Importing || status == QueueStatus::Ready || host().chain().isKnown(h)) {
					cwarn << "---" << (unsigned)p.second << ":" << h << " known";
					knowns++;
				}
				else if (status == QueueStatus::Bad)
				{
					cwarn << "block hash bad!" << h << ". Bailing...";
					return;
				}
				else if (status == QueueStatus::Future) { //�˴���ΪFuture������Ϊ��֪
					cwarn << "have same future!";
					knowns++;
				}
				else if (status == QueueStatus::Unknown)
				{
					cwarn << "---" << (unsigned)p.second << ":" << h << " unknown";
					unknowns++;
					if (p.second > maxHeight)
					{
						maxHeight = (unsigned)p.second;
						_peer->m_latestHash = h;
					}
				}
				else
					knowns++;
			}
			clog(NetMessageSummary) << knowns << "knowns," << unknowns << "unknowns";
			if (unknowns > 0)
			{
				cwarn << "Not syncing and new block hash discovered: syncing.";


				if ( host().bq().knownCount() < 10 )
				{ 
					//����ǰ��������Ϊ������ʼ��
					m_sync.m_syncStartBlock = m_sync.m_lastImportedBlock;
					m_sync.m_syncStartBlockHash = m_sync.m_lastImportedBlockHash;
					m_sync.m_syncLastIrrBlock = m_sync.m_lastIrreversibleBlock;

					m_sync.m_expectBlockHashForFindingCommon = _peer->m_latestHash;
					m_sync.m_expectBlockForFindingCommon = 0;
					requestPeerLatestBlockHeader(_peer);

					//�������ж�ͬ��
					switchState(SyncState::FindingCommonBlock);
				}
			}
		}
		 
 
		 



		EthereumHost& DefaultSyncState::host()
		{
			return m_sync.host();
		}

		EthereumHost const& DefaultSyncState::host() const
		{
			return m_sync.host();
		}

		dev::eth::BlockQueue& DefaultSyncState::bq()
		{
			return m_sync.host().bq();
		}

		dev::eth::BlockQueue const& DefaultSyncState::bq() const
		{
			return m_sync.host().bq();
		}

		bool DefaultSyncState::haveBlockHeader(uint32_t _num)
		{
			return haveItem(m_sync.m_headers, _num);
		}

		void DefaultSyncState::switchState(SyncState _s)
		{
			m_sync.switchState(_s);
		}

		void DefaultSyncState::clearPeerDownloadMarks(std::shared_ptr<EthereumPeer> _peer)
		{
			m_sync.clearPeerDownload(_peer);
		}

		bool DefaultSyncState::peerLegalCheck(std::shared_ptr<EthereumPeer> _peer)
		{
			std::shared_ptr<SessionFace> session = _peer->session();
			if (!session)
				return false; // Expired 

			_peer->setLlegal(false);

			if (_peer->m_genesisHash != host().chain().genesisHash())
				_peer->disable("Invalid genesis hash");
			else if (_peer->m_protocolVersion != host().protocolVersion() && _peer->m_protocolVersion != EthereumHost::c_oldProtocolVersion)
				_peer->disable("Invalid protocol version.");
			else if (_peer->m_networkId != host().networkId())
				_peer->disable("Invalid network identifier.");
			else if (session->info().clientVersion.find("/v0.7.0/") != string::npos)
				_peer->disable("Blacklisted client version.");
			else if (host().isBanned(session->id()))
				_peer->disable("Peer banned for previous bad behaviour.");
			else if (_peer->m_asking != Asking::State && _peer->m_asking != Asking::Nothing)
				_peer->disable("Peer banned for unexpected status message.");
			else
			{
				if (_peer->m_lastIrrBlock <= m_sync.m_lastIrreversibleBlock)
				{
					if( _peer->m_lastIrrBlockHash != host().chain().numberHash(_peer->m_lastIrrBlock) )
					{//���ִ�Peer���е����뵱ǰ����ͬ��ֱ�ӶϿ�����
						cwarn<< _peer->id()<<"|" << "Peer Last Irr Block Illegal!!!";
						_peer->disable("Peer Last Irr Block Illegal!!!");
						return false;
					}
				}

				_peer->setLlegal(true);
				return true;
			}

			return false;
		}

		bool DefaultSyncState::checkSyncComplete() const
		{ 
			if (m_sync.m_headers.empty())
			{
				assert(m_sync.m_bodies.empty()); 
				return true;
			}
			return false; 
		}

		void DefaultSyncState::resetSyncTempData()
		{
			m_sync.m_downloadingHeaders.clear();
			m_sync.m_downloadingBodies.clear();
			m_sync.m_headers.clear();
			m_sync.m_bodies.clear();
			m_sync.m_headerSyncPeers.clear();
			m_sync.m_bodySyncPeers.clear();
			m_sync.m_headerIdToNumber.clear();
		}


		void DefaultSyncState::resetAllSyncData()
		{
			resetSyncTempData();
			bq().clear(); 
		}

		void DefaultSyncState::printBlockHeadersInfo(RLP const& _r)
		{
			//��ȡheader����
			size_t itemCount = _r.itemCount();

			h256s itemHashes;
			vector<unsigned> itemNums;
			for (unsigned i = 0; i < itemCount; i++)
			{
				BlockHeader info(_r[i].data(), HeaderData);
				itemNums.push_back(static_cast<unsigned>(info.number()));
				itemHashes.push_back(info.hash());
			}

			if (itemNums.size() > 0)
			{
				cwarn << "Header Nums:" << itemNums;
				cwarn << "Header Hashes:" << itemHashes;
			}
		}

		void DefaultSyncState::requestPeerLatestBlockHeader(std::shared_ptr<EthereumPeer> _peer)
		{
			if (_peer->m_asking != Asking::Nothing)
			{
				clog(NetAllDetail) << "Can't sync with this peer - outstanding asks.";
				return;
			}

			//�ܾ��ѱ��ж�Ϊ�Ƿ���Peer
			if (!_peer->isLlegal())
				return; 

			_peer->requestBlockHeaders(_peer->m_latestHash, 1, 0, false);
			_peer->m_requireTransactions = true; 
 
		}
		 

		void DefaultSyncState::onPeerStatus(std::shared_ptr<EthereumPeer> _peer)
		{
			//һ�������ֻ��Peer���Ϸ��Լ��
			peerLegalCheck(_peer); 

			updateTimeout();

		}

		void DefaultSyncState::onPeerBlockHeaders(std::shared_ptr<EthereumPeer> _peer, RLP const& _r)
		{
			updateTimeout();
		}

		void DefaultSyncState::onPeerBlockBodies(std::shared_ptr<EthereumPeer> _peer, RLP const& _r)
		{
			updateTimeout();
		}

		void DefaultSyncState::onPeerNewBlock(std::shared_ptr<EthereumPeer> _peer, RLP const& _r)
		{ 
			updateTimeout();

			if (_r.itemCount() != 3)
			{
				_peer->disable("NewBlock without 2 data fields.");
				return;
			}

			BlockHeader header(_r[0][0].data(), HeaderData);
			auto h = header.hash();  
			cwarn << "onPeerNewBlock ==>" << h; 
			//�����ݰ��л�ȡ��peer�Ĳ�����ת���
			uint32_t lastIrrBlock = _r[1].toInt<u256>().convert_to<uint32_t>();
			h256 lastIrrBlockHash = _r[2].toHash<h256>();

			cwarn << "LastIrr = " << lastIrrBlock;  
			//����peer���²�����ת���
			_peer->setLastIrrBlock(lastIrrBlock);
			_peer->setLastIrrBlockHash(lastIrrBlockHash);

			//����Peer���µ�Hash
			_peer->m_latestHash = h;

			if (lastIrrBlock < m_sync.m_syncLastIrrBlock)
			{//�ܾ����ղ�����תС�ڵ�ǰ�ͻ��˵Ŀ�

				cwarn << _peer->id() << "|" << lastIrrBlock << " < " << "m_lastIrreversibleBlock = " << m_sync.m_lastIrreversibleBlock << "ignore new block!!!!";
				_peer->addRating(-10000); 
				return;
			}

			//ֻ����BlockQueueδ֪�Ŀ�
			auto status = host().bq().blockStatus(h);
			if (status != QueueStatus::Unknown)
			{
				return;
			}

			_peer->tryInsertPeerKnownBlockList(h);  
			unsigned blockNumber = static_cast<unsigned>(header.number()); 
			if (blockNumber <= m_sync.m_syncLastIrrBlock)
			{//ֻ�����Ŵ��ڲ�����Ŀ�
				return;
			}

			switch (host().bq().import(_r[0].data()))
			{
				case ImportResult::Success:
					_peer->addRating(100); 
					//�ӵ������Ŀ飬���򵥵ļ�����ֱ�ӹ㲥��ȥ
					host().pushDeliverBlock(h, _r[0].data().toBytes(), lastIrrBlock, lastIrrBlockHash);
					break;
				case ImportResult::FutureTimeKnown: 
					//�ӵ������Ŀ飬���򵥵ļ�����ֱ�ӹ㲥��ȥ
					host().pushDeliverBlock(h, _r[0].data().toBytes(), lastIrrBlock, lastIrrBlockHash);
					break;

				case ImportResult::Malformed:
				case ImportResult::BadChain: 
					_peer->disable("Malformed block received.");
					return;

				case ImportResult::AlreadyInChain:
				case ImportResult::AlreadyKnown:
					break;

				case ImportResult::FutureTimeUnknown:
				case ImportResult::UnknownParent: 
					break; 
				case ImportResult::Irreversible: //������δ֪�Ĳ�����ת�飬˵��ĳ�ͻ����뵱ǰ�ͻ���������ƫ��
					cwarn << "Unknown irreversible block founded!!!"; 
					_peer->addRating(-10000);
					_peer->disable("Unknown irreversible block founded!!!");
					break;
				default:;
			}
		}

		void DefaultSyncState::onPeerNewHashes(std::shared_ptr<EthereumPeer> _peer, std::vector<std::pair<h256, u256>> const& _hashes)
		{
			updateTimeout();
		}

		void DefaultSyncState::onPeerAborting()
		{
			updateTimeout();
		}

		void DefaultSyncState::onBlockImported(BlockHeader const& _info, const uint32_t _last_irr_block, const h256& _last_irr_block_hash)
		{
			if (_info.number() > m_sync.m_lastImportedBlock)
			{
				m_sync.m_lastImportedBlock = static_cast<unsigned>(_info.number());
				m_sync.m_lastImportedBlockHash = _info.hash();
				m_sync.m_highestBlock = max(m_sync.m_lastImportedBlock, m_sync.m_highestBlock);
			}

			if (_last_irr_block > m_sync.m_lastIrreversibleBlock)
			{
				m_sync.m_lastIrreversibleBlock = _last_irr_block;
				m_sync.m_lastImportedBlockHash = _last_irr_block_hash;
			}
		}  

		void SyncBlocksSyncState::onPeerStatus(std::shared_ptr<EthereumPeer> _peer)
		{
			DefaultSyncState::onPeerStatus(_peer);
			keepAlive();
		}

		void SyncBlocksSyncState::onPeerBlockHeaders(std::shared_ptr<EthereumPeer> _peer, RLP const& _r)
		{
			m_lastBlockMsgTimePoint = fc::time_point::now();

			if (updateTimeout())
			{
				return;
			}

			cwarn << "onPeerBlockHeaders==============================";
			//��ȡheader����
			size_t itemCount = _r.itemCount();
			cwarn << "BlocksHeaders (" << dec << itemCount << "entries)" << (itemCount ? "" : ": NoMoreHeaders");


			clearPeerDownloadMarks(_peer); 
			 
			if (itemCount == 0)
			{
				cwarn << "Peer does not have the blocks requested !";
				_peer->addRating(-100);
			}

			printBlockHeadersInfo(_r);

			for (unsigned i = 0; i < itemCount; i++)
			{//����ÿһ��header

				BlockHeader header(_r[i].data(), HeaderData);
				unsigned blockNumber = static_cast<unsigned>(header.number());

				if (blockNumber <= m_sync.m_syncStartBlock)
				{
					clog(NetMessageSummary) << "Skipping header : blockNumber <= m_syncStartBlock " << blockNumber;
					continue;
				}

				if (blockNumber < m_sync.m_syncLastIrrBlock)
				{
					clog(NetMessageSummary) << "Skipping too old header " << blockNumber;
					continue;
				}

				if (haveItem(m_sync.m_headers, blockNumber))
				{//header�Ѵ���
					clog(NetMessageSummary) << "Skipping header : already haveItem" << blockNumber;
					continue;
				}  
					
				if (!checkHeader(header))
				{//ǰ��header�����⣬�л���Idleģʽ
					switchState(SyncState::Idle); 
					return;
				}
				 
				BlockChainSync::Header hdr{ _r[i].data().toBytes(), header.hash(), header.parentHash() };
				BlockChainSync::HeaderId headerId{ header.transactionsRoot(), header.sha3Uncles() };

				mergeInto(m_sync.m_headers, blockNumber, std::move(hdr));
				if (headerId.transactionsRoot == EmptyTrie && headerId.uncles == EmptyListSHA3)
				{//�ս����壬��ֱ������һ���տ��弴��
					RLPStream r(2);
					r.appendRaw(RLPEmptyList);
					r.appendRaw(RLPEmptyList);
					bytes body;
					r.swapOut(body);
					mergeInto(m_sync.m_bodies, blockNumber, std::move(body));
				}
				else
					m_sync.m_headerIdToNumber[headerId] = blockNumber; 
				 
			}//end of for 
			

			if (checkSyncComplete())
			{
				switchState(SyncState::Idle);
				return;
			}

			if (!collectBlocks())
			{//ƴ�ӵ����ʱ���ִ�������ͬ��
				switchState(SyncState::Idle);
				return;
			}

			if (checkSyncComplete())
			{
				switchState(SyncState::Idle);
				return;
			}

			//����ͬ��
			continueSync();
		}

		void SyncBlocksSyncState::syncHeadersAndBodies()
		{
			if (host().bq().knownCount() > c_blockQueueGap)
			{
				clog(NetAllDetail) << "host().bq().knownCount() > c_blockQueueGap Waiting for block queue before downloading blocks";
				resetSyncTempData();
				switchState(SyncState::Idle);
				return;
			}

			//�����²�����߶�����
			host().foreachPeerByLastIrr([this](std::shared_ptr<EthereumPeer> _peer)
			{
				if (_peer->m_asking != Asking::Nothing)
				{
					clog(NetAllDetail) << "Can't sync with this peer - outstanding asks.";
					return true;
				}

				//�ܾ��ѱ��ж�Ϊ�Ƿ���Peer
				if (!_peer->isLlegal())
					return true;

				clearPeerDownloadMarks(_peer);

				if (host().bq().knownFull())
				{
					clog(NetAllDetail) << "Waiting for block queue before downloading blocks";
					resetSyncTempData();
					switchState(SyncState::Idle);
					return false;
				}

				syncHeadersAndBodies(_peer);
				
				return false; //ֻ������߲�����߶ȵĽڵ�
			});
		}



		void SyncBlocksSyncState::syncHeadersAndBodies(std::shared_ptr<EthereumPeer> _peer)
		{ 
			// check to see if we need to download any block bodies first
			auto header = m_sync.m_headers.begin();
			h256s neededBodies;
			vector<unsigned> neededNumbers;
			vector<BlockChainSync::HeaderId> neededHeaderIds;

			unsigned index = 0;

			if (!m_sync.m_headers.empty() && m_sync.m_headers.begin()->first == m_sync.m_syncStartBlock + 1)
			{//������ͬ�飬��header��Ϊ�գ���m_syncStartBlock����

				while (header != m_sync.m_headers.end() && neededBodies.size() < c_maxRequestBodies && index < header->second.size())
				{
					unsigned block = header->first + index;
					if (m_sync.m_downloadingBodies.count(block) == 0 && !haveItem(m_sync.m_bodies, block))
					{

						neededBodies.push_back(header->second[index].hash);
						neededNumbers.push_back(block);

						BlockHeader h(header->second[index].data, HeaderData); 
						neededHeaderIds.push_back(BlockChainSync::HeaderId{ h.transactionsRoot(), h.sha3Uncles() });

						m_sync.m_downloadingBodies.insert(block);
					}

					++index;
					if (index >= header->second.size())
						break; // Download bodies only for validated header chain
				}
			}
			if (neededBodies.size() > 0)
			{
				cwarn << "request Block Nums:" << neededNumbers;
				cwarn << "request Block Hashes:" << neededBodies;

				for (int i = 0; i < neededHeaderIds.size(); i++)
				{
					if (m_sync.m_headerIdToNumber.find(neededHeaderIds[i]) == m_sync.m_headerIdToNumber.end())
					{
						m_sync.m_headerIdToNumber[neededHeaderIds[i]] = neededNumbers[i];
					}
				}

				m_sync.m_bodySyncPeers[_peer] = neededNumbers;
				_peer->requestBlockBodies(neededBodies);
			}
			else
			{
				// check if need to download headers
				unsigned start = m_sync.m_syncStartBlock + 1;
				auto next = m_sync.m_headers.begin();
				unsigned count = 0;
				if (!m_sync.m_headers.empty() && start >= m_sync.m_headers.begin()->first)
				{
					start = m_sync.m_headers.begin()->first + m_sync.m_headers.begin()->second.size();
					++next;
				}

				while (count == 0 && next != m_sync.m_headers.end())
				{
					count = std::min(c_maxRequestHeaders, next->first - start);
					while (count > 0 && m_sync.m_downloadingHeaders.count(start) != 0)
					{
						start++;
						count--;
					}
					std::vector<unsigned> headers;
					for (unsigned block = start; block < start + count; block++)
						if (m_sync.m_downloadingHeaders.count(block) == 0)
						{
							headers.push_back(block);
							m_sync.m_downloadingHeaders.insert(block);
						}
					count = headers.size();
					if (count > 0)
					{
						m_sync.m_headerSyncPeers[_peer] = headers;
						assert(!haveItem(m_sync.m_headers, start));
						_peer->requestBlockHeaders(start, count, 0, false);
					}
					else if (start >= next->first)
					{
						start = next->first + next->second.size();
						++next;
					}
				} 
			} 
		}

		void SyncBlocksSyncState::onPeerBlockBodies(std::shared_ptr<EthereumPeer> _peer, RLP const& _r)
		{ 
			m_lastBlockMsgTimePoint = fc::time_point::now();

			if (updateTimeout())
			{
				return;
			}

			size_t itemCount = _r.itemCount();
			cwarn << "BlocksBodies (" << dec << itemCount << "entries)" << (itemCount ? "" : ": NoMoreBodies");

			clearPeerDownloadMarks(_peer); 

			if (itemCount == 0)
			{
				cwarn << "Peer does not have the blocks requested";
				_peer->addRating(-100);
			}

			//���ڴ�ӡ��־
			std::vector<unsigned> bodies;

			for (unsigned i = 0; i < itemCount; i++)
			{
				RLP body(_r[i]);

				auto txList = body[0];
				h256 transactionRoot = trieRootOver(txList.itemCount(), [&](unsigned i) { return rlp(i); }, [&](unsigned i) { return txList[i].data().toBytes(); });
				h256 uncles = sha3(body[1].data());
				BlockChainSync::HeaderId id{ transactionRoot, uncles };
				auto iter = m_sync.m_headerIdToNumber.find(id);
				if (iter == m_sync.m_headerIdToNumber.end() || !haveItem(m_sync.m_headers, iter->second))
				{   
					cwarn << "Ignored unknown block body";
					continue;
				}
				unsigned blockNumber = iter->second;

				bodies.push_back(blockNumber);

				if (haveItem(m_sync.m_bodies, blockNumber))
				{
					cwarn << "Skipping already downloaded block body " << blockNumber;
					continue;
				}
				m_sync.m_headerIdToNumber.erase(id); 
				mergeInto(m_sync.m_bodies, blockNumber, body.data().toBytes());
			}

			cwarn << "BlockBodies: " << bodies;

			if (checkSyncComplete())
			{
				switchState(SyncState::Idle);
				return;
			}

			if (!collectBlocks())
			{//ƴ�ӵ����ʱ���ִ�������ͬ��
				switchState(SyncState::Idle);
				return;
			}

			if (checkSyncComplete())
			{
				switchState(SyncState::Idle);
				return;
			}
			 
			continueSync();
		}

 


		void SyncBlocksSyncState::onPeerNewBlock(std::shared_ptr<EthereumPeer> _peer, RLP const& _r)
		{
			DefaultSyncState::onPeerNewBlock(_peer, _r);
			keepAlive();
		}

		void SyncBlocksSyncState::onPeerNewHashes(std::shared_ptr<EthereumPeer> _peer, std::vector<std::pair<h256, u256>> const& _hashes)
		{
			DefaultSyncState::onPeerNewHashes(_peer, _hashes);
			keepAlive();
		}

		void SyncBlocksSyncState::onPeerAborting()
		{
			DefaultSyncState::onPeerAborting();
			keepAlive();
		}

		void SyncBlocksSyncState::onEnter()
		{
			updateLastUpdateTime(); 
			//��״̬����ʱ��Ҫ����ͬ��
			continueSync();
		}

		void SyncBlocksSyncState::continueSync()
		{ 
			syncHeadersAndBodies();
		}

		void SyncBlocksSyncState::keepAlive()
		{
			double dt = ((double)((fc::time_point::now() - m_lastBlockMsgTimePoint).to_milliseconds())) / 1000.0;
			if (dt > 5.0)
			{
				continueSync();
				m_lastBlockMsgTimePoint = fc::time_point::now();
			}
		}

		void SyncBlocksSyncState::onTimeout()
		{
			switchState(SyncState::Idle);
		}

		bool SyncBlocksSyncState::checkHeader(const BlockHeader& _h)
		{ 
			unsigned int blockNumber = _h.number().convert_to<unsigned int>();

			// validate chain
			BlockChainSync::HeaderId headerId{ _h.transactionsRoot(), _h.sha3Uncles() }; 
			BlockChainSync::Header const* prevBlock = findItem(m_sync.m_headers, blockNumber - 1);

			if (
				(prevBlock && prevBlock->hash != _h.parentHash()) ||
				(blockNumber == m_sync.m_syncStartBlock + 1 && _h.parentHash() != m_sync.m_syncStartBlockHash))
			{
				cwarn << "Block header mismatching parent!!!";
				if (prevBlock == NULL)
				{
					cwarn << "prevBlock == NULL";
				}
				else {
					cwarn << "prevBlock->hash = " << prevBlock->hash << " info.parentHash() = " << _h.parentHash();
				}

				cwarn << "blockNumber = " << blockNumber << " m_syncStartBlock = " << m_sync.m_syncStartBlock << " m_syncStartBlockHash = " << m_sync.m_syncStartBlockHash;
				// mismatching parent id, delete the previous block and don't add this one
				clog(NetImpolite) << "Unknown block header " << blockNumber << " " << _h.hash() << " (Restart syncing)";

				
				resetAllSyncData();
				return false;
			}

			BlockChainSync::Header const* nextBlock = findItem(m_sync.m_headers, blockNumber + 1);
			if (nextBlock && nextBlock->parent != _h.hash())
			{
				cwarn << "Block header mismatching next block!!!"; 
				clog(NetImpolite) << "Unknown block header " << blockNumber + 1 << " " << nextBlock->hash;
				// clear following headers
				unsigned n = blockNumber + 1;
				auto headers = m_sync.m_headers.at(n);
				for (auto const& h : headers)
				{
					BlockHeader deletingInfo(h.data, HeaderData);
					m_sync.m_headerIdToNumber.erase(headerId);
					m_sync.m_downloadingBodies.erase(n);
					m_sync.m_downloadingHeaders.erase(n);
					++n;
				}
				removeAllStartingWith(m_sync.m_headers, blockNumber + 1);
				removeAllStartingWith(m_sync.m_bodies, blockNumber + 1);
			}
			return true;
		}

		bool SyncBlocksSyncState::collectBlocks()
		{ 
			// merge headers and bodies
			auto& headers = *m_sync.m_headers.begin();
			auto& bodies = *m_sync.m_bodies.begin();
			if (headers.first != bodies.first || headers.first != m_sync.m_syncStartBlock + 1)
				return true;

			unsigned success = 0;
			unsigned future = 0;
			unsigned got = 0;
			unsigned unknown = 0;
			size_t i = 0;
			for (; i < headers.second.size() && i < bodies.second.size(); i++)
			{
				cwarn << "try block import : num = "<< headers.first + i << " hash = "<<headers.second[i].hash;
				RLPStream blockStream(3);
				blockStream.appendRaw(headers.second[i].data);
				RLP body(bodies.second[i]);
				blockStream.appendRaw(body[0].data());
				blockStream.appendRaw(body[1].data());
				bytes block;
				blockStream.swapOut(block);
				switch (host().bq().import(&block))
				{
				case ImportResult::Success:
					success++;
					if (headers.first + i > m_sync.m_syncStartBlock)
					{
						m_sync.m_syncStartBlock = headers.first + (unsigned)i;
						m_sync.m_syncStartBlockHash = headers.second[i].hash;
					}
					break;
				case ImportResult::Malformed:
				case ImportResult::BadChain: 
					resetAllSyncData();
					return false;

				case ImportResult::FutureTimeKnown:
					future++; 
					if (headers.first + i > m_sync.m_syncStartBlock)
					{
						m_sync.m_syncStartBlock = headers.first + (unsigned)i;
						m_sync.m_syncStartBlockHash = headers.second[i].hash;
					}
					break;
				case ImportResult::AlreadyInChain: 
				case ImportResult::AlreadyKnown: 
					if (headers.first + i > m_sync.m_syncStartBlock)
					{
						m_sync.m_syncStartBlock = headers.first + (unsigned)i;
						m_sync.m_syncStartBlockHash = headers.second[i].hash;
					}
					break;
				case ImportResult::FutureTimeUnknown:
				case ImportResult::UnknownParent:
					if (headers.first + i > m_sync.m_syncStartBlock)
					{
						resetAllSyncData();
						return false;
					}
					return true;

				case ImportResult::Irreversible: //������δ֪�Ĳ�����ת�飬˵��ĳ�ͻ����뵱ǰ�ͻ���������ƫ��
					cwarn << "Unknown irreversible block founded!!! ignore and restart sync!"; 
					resetAllSyncData();
					return false;
				default:;
				}
			}

			clog(NetMessageSummary) << dec << success << "imported OK," << unknown << "with unknown parents," << future << "with future timestamps," << got << " already known received.";

			if (host().bq().unknownFull())
			{//̫��δ֪�飬��Ҫ����ͬ�� 
				clog(NetWarn) << "Too many unknown blocks, restarting sync"; 
				resetAllSyncData();
				return false;
			}

			auto newHeaders = std::move(headers.second);
			newHeaders.erase(newHeaders.begin(), newHeaders.begin() + i);
			unsigned newHeaderHead = headers.first + i;
			auto newBodies = std::move(bodies.second);
			newBodies.erase(newBodies.begin(), newBodies.begin() + i);
			unsigned newBodiesHead = bodies.first + i;
			m_sync.m_headers.erase(m_sync.m_headers.begin());
			m_sync.m_bodies.erase(m_sync.m_bodies.begin());
			if (!newHeaders.empty())
				m_sync.m_headers[newHeaderHead] = newHeaders;
			if (!newBodies.empty())
				m_sync.m_bodies[newBodiesHead] = newBodies;

			return true;  
		}

		void FindingCommonBlockSyncState::onEnter()
		{
			m_unexpectTimes = 0;
			m_lastBlockHeaderTimePoint = fc::time_point::now();
			updateLastUpdateTime();
		}

		void FindingCommonBlockSyncState::onPeerStatus(std::shared_ptr<EthereumPeer> _peer)
		{ 
			DefaultSyncState::onPeerStatus(_peer);
			keepAlive();
		}

		void FindingCommonBlockSyncState::onPeerBlockHeaders(std::shared_ptr<EthereumPeer> _peer, RLP const& _r)
		{
			m_lastBlockHeaderTimePoint = fc::time_point::now();

			if (updateTimeout())
			{//������ʱ
				return;
			}

			cwarn << "onPeerBlockHeaders==============================";
			//��ȡheader����
			size_t itemCount = _r.itemCount();

			clog(NetMessageSummary) << "BlocksHeaders (" << dec << itemCount << "entries)" << (itemCount ? "" : ": NoMoreHeaders"); 

			printBlockHeadersInfo(_r); 

			clearPeerDownloadMarks(_peer); 

			bool commonHeaderFounded = false;

			if (itemCount == 0)
			{
				cwarn << "Peer does not have the blocks requested !";
				_peer->addRating(-100);
				return;
			}
			else if( itemCount == 1)
			{//ֻ�Ե�����ͷ����Ϣ���д���,��Ϊ�ڲ�����ͬ��׶�ֻ���е�������Ϣ����

				BlockHeader header(_r[0].data(), HeaderData); 
				unsigned blockNumber = static_cast<unsigned>(header.number()); 
				
				if (isExpectBlockHeader(header))
				{
					if (blockNumber < m_sync.m_syncLastIrrBlock )
					{//Ԥ�ڿ�߶�С�ڲ����棬ֱ���˻�Idle״̬  
						switchState(SyncState::Idle);
						return;
					}

					if (!haveItem(m_sync.m_headers, blockNumber))
					{
						if (importBlockHeader(_r))
						{//�ҵ�CommonHeader

							if (checkSyncComplete())
							{//˵������ĵ�һ���鼴Ϊ��֪��
								switchState(SyncState::Idle);
								return;
							}

							//�л���SyncBlocks���н�һ��ͬ�� 
							switchState(SyncState::SyncBlocks); 
							return;
						}
						else {//��ͷ�ѵ��룬������Common��������ǰ��

							if (blockNumber == m_sync.m_syncLastIrrBlock)
							{//˵����ʱpeer������ǲ���ͬ�Ĳ�����ת��
								_peer->addRating(-10000); 
								switchState(SyncState::Idle);
								return;
							}

							moveToNextCommonBlock();
						}
					}

				} else {//�ӵ���Ԥ�ڿ�ͷ 
					cwarn << "recv unexpected block header!!!";
					m_unexpectTimes++;
					if (m_unexpectTimes > 20)
					{//�п��ܶԷ����л��ֲ�
						m_unexpectTimes = 0;
						_peer->addRating(-10000);
						switchState(SyncState::Idle);
						return;
					}
				}  
			}else {//�ش���Header������Ԥ��
				cwarn << "Ignore peer unexpected block header !";
			} 
			
			continueSync();
		}
		 

		void FindingCommonBlockSyncState::onPeerBlockBodies(std::shared_ptr<EthereumPeer> _peer, RLP const& _r)
		{
			DefaultSyncState::onPeerBlockBodies(_peer, _r);
			keepAlive();
		}

		void FindingCommonBlockSyncState::onPeerNewBlock(std::shared_ptr<EthereumPeer> _peer, RLP const& _r)
		{
			DefaultSyncState::onPeerNewBlock(_peer, _r);
			keepAlive();
		}

		void FindingCommonBlockSyncState::onPeerNewHashes(std::shared_ptr<EthereumPeer> _peer, std::vector<std::pair<h256, u256>> const& _hashes)
		{
			DefaultSyncState::onPeerNewHashes(_peer, _hashes);
			keepAlive();
		}

		void FindingCommonBlockSyncState::onPeerAborting()
		{
			DefaultSyncState::onPeerAborting(); 
		}

		void FindingCommonBlockSyncState::onTimeout()
		{
			switchState(SyncState::Idle);
		}

		unsigned int FindingCommonBlockSyncState::nextTryCommonBlock() const
		{
			unsigned start = m_sync.m_syncStartBlock;
			if (!m_sync.m_headers.empty())
				start = std::min(start, m_sync.m_headers.begin()->first - 1);
			return start;
		}

		void FindingCommonBlockSyncState::moveToNextCommonBlock()
		{
			//download backwards until common block is found 1 header at a time
			unsigned start = nextTryCommonBlock(); 
			m_sync.m_syncStartBlock = start;
			m_sync.m_syncStartBlockHash = host().chain().numberHash(start);

			m_sync.m_expectBlockForFindingCommon = start;
			m_sync.m_expectBlockHashForFindingCommon = h256();
		}

		bool FindingCommonBlockSyncState::isExpectBlockHeader(const BlockHeader& _h) const
		{
			return 
				m_sync.m_expectBlockHashForFindingCommon == _h.hash() || 
				m_sync.m_expectBlockForFindingCommon == static_cast<unsigned>(_h.number());
		}

		void FindingCommonBlockSyncState::continueSync()
		{
			if (m_sync.m_expectBlockHashForFindingCommon != h256())
			{
				requestExpectHashHeader();
			}
			else {
				requestNextCommonHeader();
			}
		}

		void FindingCommonBlockSyncState::keepAlive()
		{
			double dt = ((double)((fc::time_point::now() - m_lastBlockHeaderTimePoint).to_milliseconds())) / 1000.0;
			if (dt > 5.0)
			{
				continueSync();
				m_lastBlockHeaderTimePoint = fc::time_point::now();
			}
		}

		bool FindingCommonBlockSyncState::importBlockHeader(RLP const& _r)
		{
			BlockHeader header(_r[0].data(), HeaderData);
			unsigned blockNumber = static_cast<unsigned>(header.number());

			auto status = host().bq().blockStatus(header.hash());
			if (status == QueueStatus::Importing || status == QueueStatus::Ready || host().chain().isKnown(header.hash()))
			{//������ͬ��
				cwarn << "Block: " << header.hash() << "|" << blockNumber << " => common block header founded!";
				m_sync.m_syncStartBlock = (unsigned)header.number();
				m_sync.m_syncStartBlockHash = header.hash();
				return true;
			}
			else
			{//δ�ҵ�Common���򽫿�ͷ�����ѿ�ͷ�� 
				BlockChainSync::Header hdr{ _r[0].data().toBytes(), header.hash(), header.parentHash() };
				BlockChainSync::HeaderId headerId{ header.transactionsRoot(), header.sha3Uncles() };

				mergeInto(m_sync.m_headers, blockNumber, std::move(hdr));
				if (headerId.transactionsRoot == EmptyTrie && headerId.uncles == EmptyListSHA3)
				{//�ս����壬��ֱ������һ���տ��弴��
					RLPStream r(2);
					r.appendRaw(RLPEmptyList);
					r.appendRaw(RLPEmptyList);
					bytes body;
					r.swapOut(body);
					mergeInto(m_sync.m_bodies, blockNumber, std::move(body));
				}
				else
					m_sync.m_headerIdToNumber[headerId] = blockNumber;
			}
			return false;
		}

		

		void FindingCommonBlockSyncState::requestNextCommonHeader()
		{
			host().foreachPeerByLastIrr([this](std::shared_ptr<EthereumPeer> _p)
			{
				if (_p->m_asking != Asking::Nothing)
				{
					clog(NetAllDetail) << "Can't sync with this peer - outstanding asks.";
					return true;
				}

				//�ܾ��ѱ��ж�Ϊ�Ƿ���Peer
				if (!_p->isLlegal())
					return true;

				clearPeerDownloadMarks(_p);


				_p->requestBlockHeaders(nextTryCommonBlock(), 1, 0, false);
				return false;
			});
		}

		void FindingCommonBlockSyncState::requestExpectHashHeader()
		{
			host().foreachPeerByLastIrr([this](std::shared_ptr<EthereumPeer> _p)
			{
				if (_p->m_asking != Asking::Nothing)
				{
					clog(NetAllDetail) << "Can't sync with this peer - outstanding asks.";
					return true;
				}

				//�ܾ��ѱ��ж�Ϊ�Ƿ���Peer
				if (!_p->isLlegal())
					return true; 
				clearPeerDownloadMarks(_p); 

				_p->requestBlockHeaders(m_sync.m_expectBlockHashForFindingCommon, 1, 0, false);
				return false;
			});
		}

 

	}
}