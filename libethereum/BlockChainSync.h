/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file BlockChainSync.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <mutex>
#include <unordered_map>

#include <libdevcore/Guards.h>
#include <libethcore/Common.h>
#include <libethcore/BlockHeader.h>
#include <libp2p/Common.h>
#include "CommonNet.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "BlockChainSyncState.h"

namespace dev
{

class RLPStream;

namespace eth
{

class EthereumHost;
class BlockQueue;
class EthereumPeer;

/**
 * @brief Base BlockChain synchronization strategy class.
 * Syncs to peers and keeps up to date. Base class handles blocks downloading but does not contain any details on state transfer logic.
 */
class BlockChainSync final: public HasInvariants
{
public:
	BlockChainSync(EthereumHost& _host);
	~BlockChainSync();

	//Ŀǰ������ʼ�����²�����ת���
	void init(const uint32_t _last_irr_block,const h256& _last_irr_block_hash);

	void abortSync(); ///< Abort all sync activity

	/// @returns true is Sync is in progress
	bool isSyncing() const;

	/// Restart sync
	void restartSync();

	/// Called after all blocks have been downloaded
	/// Public only for test mode
	void completeSync();

	/// Called by peer to report status
	void onPeerStatus(std::shared_ptr<EthereumPeer> _peer);

	/// Called by peer once it has new block headers during sync
	void onPeerBlockHeaders(std::shared_ptr<EthereumPeer> _peer, RLP const& _r);

	/// Called by peer once it has new block bodies
	void onPeerBlockBodies(std::shared_ptr<EthereumPeer> _peer, RLP const& _r);

	/// Called by peer once it has new block bodies
	void onPeerNewBlock(std::shared_ptr<EthereumPeer> _peer, RLP const& _r);

	void onPeerNewHashes(std::shared_ptr<EthereumPeer> _peer, std::vector<std::pair<h256, u256>> const& _hashes);

	/// Called by peer when it is disconnecting
	void onPeerAborting();

	/// Called when a blockchain has imported a new block onto the DB
	void onBlockImported(BlockHeader const& _info, const uint32_t _last_irr_block, const h256& _last_irr_block_hash);

	/// @returns Synchonization status
	SyncStatus status() const;

	static char const* stateName(SyncState _s);

	void switchState(SyncState _s);

private: 

	/// Enter waiting state
	void pauseSync();

	EthereumHost& host() { return m_host; }
	EthereumHost const& host() const { return m_host; }

	void resetSync(); 
	void clearPeerDownload(std::shared_ptr<EthereumPeer> _peer);
	void clearPeerDownload();
	void collectBlocks();  

	void back2LastIrrBlockAndResync();
	void keeySyncAlive();
private:
	struct Header
	{
		bytes data;		///< Header data
		h256 hash;		///< Block hash
		h256 parent;	///< Parent hash
	};

	/// Used to identify header by transactions and uncles hashes
	struct HeaderId
	{
		h256 transactionsRoot;
		h256 uncles;

		bool operator==(HeaderId const& _other) const
		{
			return transactionsRoot == _other.transactionsRoot && uncles == _other.uncles;
		}
	};

	struct HeaderIdHash
	{
		std::size_t operator()(const HeaderId& _k) const
		{
			size_t seed = 0;
			h256::hash hasher;
			boost::hash_combine(seed, hasher(_k.transactionsRoot));
			boost::hash_combine(seed, hasher(_k.uncles));
			return seed;
		}
	};

	EthereumHost& m_host;
	Handler<> m_bqRoomAvailable;				///< Triggered once block queue has space for more blocks
	mutable RecursiveMutex x_sync; 
	std::atomic<SyncState> m_state{SyncState::Idle};		///< Current sync state 

	std::unordered_set<unsigned> m_downloadingHeaders;		///< Set of block body numbers being downloaded
	std::unordered_set<unsigned> m_downloadingBodies;		///< Set of block header numbers being downloaded
	std::map<unsigned, std::vector<Header>> m_headers;	    ///< Downloaded headers
	std::map<unsigned, std::vector<bytes>> m_bodies;	    ///< Downloaded block bodies
	std::map<std::weak_ptr<EthereumPeer>, std::vector<unsigned>, std::owner_less<std::weak_ptr<EthereumPeer>>> m_headerSyncPeers; ///< Peers to m_downloadingSubchain number map
	std::map<std::weak_ptr<EthereumPeer>, std::vector<unsigned>, std::owner_less<std::weak_ptr<EthereumPeer>>> m_bodySyncPeers; ///< Peers to m_downloadingSubchain number map
	std::unordered_map<HeaderId, unsigned, HeaderIdHash> m_headerIdToNumber;
	unsigned m_lastImportedBlock = 0; 			///< Last imported block number
	h256 m_lastImportedBlockHash;				///< Last imported block hash 
	unsigned m_lastIrreversibleBlock = 0;		///<��ǰ�����²�����ת��
	h256 m_lastIrreversibleBlockHash;			///<��ǰ�����²�����ת�� hash

	/*����ͬ�������״̬����*/
	unsigned m_syncStartBlock = 0;				
	h256	 m_syncStartBlockHash;
	unsigned m_syncLastIrrBlock = 0;

	unsigned m_expectBlockForFindingCommon = 0;
	h256	 m_expectBlockHashForFindingCommon = h256();


	///////////////>�����ֶ�
	h256Hash m_knownNewHashes;
	///< New hashes we know about use for logging only
	unsigned m_chainStartBlock = 0;
	unsigned m_startingBlock = 0;      	    	///< Last block number for the start of sync
	unsigned m_highestBlock = 0;       	     	///< Highest block number seen 
	bool m_haveCommonHeader = false;			///< True if common block for our and remote chain has been found
	///////////////>�����ֶ�

private:
	//static char const* const s_stateNames[static_cast<int>(SyncState::Size)];
	bool invariants() const override;
	void logNewBlock(h256 const& _h);


	friend class BlockChainSyncState;
	friend class NotSyncedState; 
	friend class IdleSyncState; 
	friend class WaitingSyncState; 
	friend class BlockSyncState;
	friend class FindingCommonBlockSyncState;
	friend class SyncBlocksSyncState; 
	friend class DefaultSyncState;

	//��ǰ״̬
	std::shared_ptr<BlockChainSyncState> m_pCurrState;

	std::shared_ptr<NotSyncedState> m_pNotSyncedState;
	std::shared_ptr<IdleSyncState> m_pIdleSyncState;
	std::shared_ptr<WaitingSyncState> m_pWatingSyncState;
	std::shared_ptr<BlockSyncState> m_pBlockSyncState;
	std::shared_ptr<FindingCommonBlockSyncState> m_pFindingCommonBlockSyncState;
	std::shared_ptr<SyncBlocksSyncState> m_pSyncBlocksSyncState;
	
};

std::ostream& operator<<(std::ostream& _out, SyncStatus const& _sync);

}
}
