#pragma once

#include <mutex>
#include <unordered_map>

#include <libdevcore/Guards.h>
#include <libethcore/Common.h>
#include <libethcore/BlockHeader.h>
#include <libp2p/Common.h>
#include "CommonNet.h"

namespace dev
{ 
	class RLPStream;

	namespace eth
	{
		class EthereumHost;
		class BlockQueue;
		class EthereumPeer; 
		class BlockChainSync;

		class BlockChainSyncState
		{
		public:
			BlockChainSyncState(BlockChainSync& _sync);
			 
			virtual void onPeerStatus(std::shared_ptr<EthereumPeer> _peer) = 0; 
			virtual void onPeerBlockHeaders(std::shared_ptr<EthereumPeer> _peer, RLP const& _r) = 0; 
			virtual void onPeerBlockBodies(std::shared_ptr<EthereumPeer> _peer, RLP const& _r) = 0; 
			virtual void onPeerNewBlock(std::shared_ptr<EthereumPeer> _peer, RLP const& _r) = 0; 
			virtual void onPeerNewHashes(std::shared_ptr<EthereumPeer> _peer, std::vector<std::pair<h256, u256>> const& _hashes) = 0; 
			virtual void onPeerAborting() = 0; 
			virtual void onBlockImported(BlockHeader const& _info, const uint32_t _last_irr_block) = 0;

			virtual void onEnter() = 0;
			virtual void onLeave() = 0;

		protected:
			BlockChainSync& m_sync; 
		};


		class DefaultSyncState : public BlockChainSyncState
		{
		public:
			DefaultSyncState(BlockChainSync& _sync) :BlockChainSyncState(_sync) { m_lastUpdateTime = fc::time_point::now(); }
			virtual void onPeerStatus(std::shared_ptr<EthereumPeer> _peer);
			virtual void onPeerBlockHeaders(std::shared_ptr<EthereumPeer> _peer, RLP const& _r);
			virtual void onPeerBlockBodies(std::shared_ptr<EthereumPeer> _peer, RLP const& _r);
			virtual void onPeerNewBlock(std::shared_ptr<EthereumPeer> _peer, RLP const& _r);
			virtual void onPeerNewHashes(std::shared_ptr<EthereumPeer> _peer, std::vector<std::pair<h256, u256>> const& _hashes);
			virtual void onPeerAborting();
			virtual void onBlockImported(BlockHeader const& _info, const uint32_t _last_irr_block);

			virtual void onEnter() { updateLastUpdateTime(); }
			virtual void onLeave() {}

		protected:
			/*==============��ʱ����==================*/

			void updateLastUpdateTime() 
			{ 
				m_lastUpdateTime = fc::time_point::now(); 
			}

			//����true��ʾ����timeout
			bool updateTimeout() 
			{  
				if (elapseTime() > timeoutElapseSec())
				{//��ʱ
					ctrace << "TIME OUT!!! elapse secs > " << timeoutElapseSec();
					this->onTimeout();
					return true;
				}
				m_lastUpdateTime = fc::time_point::now();
				return false;
			}

			double elapseTime() { return ((double)((fc::time_point::now() - m_lastUpdateTime).to_milliseconds())) / 1000.0; }

			virtual double timeoutElapseSec() const { return DBL_MAX; }

			virtual void onTimeout() {}
			 

			/*==============���ߺ���==================*/ 
			 
			EthereumHost& host();
			EthereumHost const& host() const;
			BlockQueue& bq();
			BlockQueue const& bq() const;

			bool haveBlockHeader(uint32_t _num);  

			void switchState(SyncState _s);

			//��յ�ǰPeer�����ؼ�¼
			void clearPeerDownloadMarks(std::shared_ptr<EthereumPeer> _peer);
			
			bool peerLegalCheck(std::shared_ptr<EthereumPeer> _peer);

			//���m_headers�Ƿ�Ϊ�գ���Ϊ���򷵻�true
			bool checkSyncComplete() const;

			//�����ʱ����
			void resetSyncTempData();

			//���ش����ʱ���ã��������ͬ������ʱ���ݻ�����bq
			void resetAllSyncData();

			void printBlockHeadersInfo(RLP const& _r); 

			/*==============Peerͬ����غ���==================*/

			virtual void requestPeerLatestBlockHeader(std::shared_ptr<EthereumPeer> _peer);

			void requestBlocks(std::shared_ptr<EthereumPeer> _peer);

			virtual void continueSync() {}

		protected:

			fc::time_point m_lastUpdateTime;
		};



		class IdleSyncState : public DefaultSyncState
		{
		public:
			IdleSyncState(BlockChainSync& _sync):DefaultSyncState(_sync) {} 
			virtual void onPeerStatus(std::shared_ptr<EthereumPeer> _peer); 
			virtual void onPeerNewBlock(std::shared_ptr<EthereumPeer> _peer, RLP const& _r);
			virtual void onPeerNewHashes(std::shared_ptr<EthereumPeer> _peer, std::vector<std::pair<h256, u256>> const& _hashes); 

			virtual void onEnter() 
			{ 
				updateLastUpdateTime();
				resetSyncTempData(); 
			}
		};
		
		
		class FindingCommonBlockSyncState : public DefaultSyncState
		{
		public:
			FindingCommonBlockSyncState(BlockChainSync& _sync) :DefaultSyncState(_sync) {}
			virtual void onPeerStatus(std::shared_ptr<EthereumPeer> _peer);
			virtual void onPeerBlockHeaders(std::shared_ptr<EthereumPeer> _peer, RLP const& _r);
			virtual void onPeerBlockBodies(std::shared_ptr<EthereumPeer> _peer, RLP const& _r);
			virtual void onPeerNewBlock(std::shared_ptr<EthereumPeer> _peer, RLP const& _r);
			virtual void onPeerNewHashes(std::shared_ptr<EthereumPeer> _peer, std::vector<std::pair<h256, u256>> const& _hashes);
			virtual void onPeerAborting();
			virtual void onEnter();
		protected: 

			//ʮ�볬ʱ
			virtual double timeoutElapseSec() const { return 10.0; }

			virtual void onTimeout();

			//������һ�����Բ��ҵ�Common��
			unsigned int nextTryCommonBlock() const; 
			//������յ��Ŀ�ͷ�������Ƿ��ҵ�Common��
			bool importBlockHeader(RLP const& _r);
			void moveToNextCommonBlock(); 
			void requestNextCommonHeader(); 
			void requestExpectHashHeader(); 
			bool isExpectBlockHeader(const BlockHeader& _h) const; 

			virtual void continueSync();

			//ÿ��һ��ʱ�䷢������BlockHeader�������FindingCommon״̬�Ļ���
			void keepAlive();
		private: 

			int	m_unexpectTimes = 0;					    ///<�յ���Ԥ�ڴ���
			fc::time_point m_lastBlockHeaderTimePoint; ///<���һ�νӵ�BlockHeader��ʱ���
		};

		class SyncBlocksSyncState : public DefaultSyncState
		{
		public:
			SyncBlocksSyncState(BlockChainSync& _sync) :DefaultSyncState(_sync) {}
			virtual void onPeerStatus(std::shared_ptr<EthereumPeer> _peer);
			virtual void onPeerBlockHeaders(std::shared_ptr<EthereumPeer> _peer, RLP const& _r);
			virtual void onPeerBlockBodies(std::shared_ptr<EthereumPeer> _peer, RLP const& _r);
			virtual void onPeerNewBlock(std::shared_ptr<EthereumPeer> _peer, RLP const& _r);
			virtual void onPeerNewHashes(std::shared_ptr<EthereumPeer> _peer, std::vector<std::pair<h256, u256>> const& _hashes);
			virtual void onPeerAborting();
			virtual void onEnter();

		protected: 

			virtual void continueSync();

			void keepAlive();

			//ʮ�볬ʱ
			virtual double timeoutElapseSec() const { return 10.0; }

			virtual void onTimeout();

			//����ͷ��ǰ�������Ƿ�Ϸ�
			//��ǰ�����Ϸ�ʱ����false�����򷵻�true
			//����̲��Ϸ�ʱ������˲��Ϸ�Header����������Header
			bool checkHeader(const BlockHeader& _h);

			//���Ժϲ���ͷ���岢����
			//����ֵΪ�Ƿ�ɹ���������false����Ҫ�л���Idle
			bool collectBlocks();

			void syncHeadersAndBodies();

			void syncHeadersAndBodies(std::shared_ptr<EthereumPeer> _peer);

		private: 
			fc::time_point m_lastBlockMsgTimePoint; ///<���һ�νӵ�BlockHeader��ʱ���
		};


		class NotSyncedState : public DefaultSyncState
		{
		public:
			NotSyncedState(BlockChainSync& _sync) :DefaultSyncState(_sync) {}
		};

		class WaitingSyncState : public DefaultSyncState
		{
		public:
			WaitingSyncState(BlockChainSync& _sync) :DefaultSyncState(_sync) {}
		};



		class BlockSyncState : public DefaultSyncState
		{
		public:
			BlockSyncState(BlockChainSync& _sync) :DefaultSyncState(_sync) {}
		};

	}
}