#pragma once
//#include <boost/multiprecision/cpp_int.hpp>
#include <libdevcore/Common.h>
#include <fc/time.hpp>
#include <libproducer/version.hpp>

namespace dev {

namespace types {
	using UInt128 = u128;
	using Uint64 = uint64_t;
	using Int64 = int64_t;
}


namespace eth {

namespace config {

	using types::UInt128;

	const static int BlockIntervalSeconds = 3;
	 
	const static int BlocksPerYear = (365 * 24 * 60 * 60 / BlockIntervalSeconds);
	const static int BlocksPerDay = (24 * 60 * 60 / BlockIntervalSeconds);
	//const static int StartMinerVotingBlock = BlocksPerDay;
	//���ڲ�����ʱ��Ϊһ��
	const static int StartMinerVotingBlock = 21;

	//ÿ��DPOS Worker����
	const static int DPOSProducersPerRound = 17;
	//ÿ�ֿ�ѡƱ�ȶ�����DPOS��֤������
	const static int DPOSVotedProducersPerRound = 16;
	//ÿ���������ܵļ�֤������
	const static int DPOSRunnerupProducersPerRound = 1;
	//ÿ��POW ��֤������
	const static int POWProducersPerRound = 4;
	//ÿ�ּ�֤������
	const static int TotalProducersPerRound = 21;


	//ETI����ʱ��
	const static fc::time_point_sec ETI_GenesisTime = (fc::time_point_sec(0));

	//ETI��ǰ�汾
	const static eth::chain::version ETI_BlockchainVersion = (eth::chain::version(0, 0, 0));

	//ETI��ǰӲ�ֲ�汾
	const static eth::chain::hardfork_version ETI_BlockchainHardforkVersion = (eth::chain::hardfork_version(ETI_BlockchainVersion));

	//ETIĿǰӲ�ֲ���
	const static int ETI_HardforkNum = 0;
	
	//Ӳ�ֲ�ͶƱ��С����
	const static int ETI_HardforkRequiredProducers = 1;

	const static int Percent100 = 10000;
	const static int Percent1 = 100;
	const static int IrreversibleThresholdPercent = 70 * Percent1;

	const static int TransactionOfBlockLimit = 10240;

	const static UInt128 ProducerRaceLapLength = std::numeric_limits<UInt128>::max();
}

template<typename Number>
Number ETH_PERCENT(Number value, int percentage) {
	return value * percentage / config::Percent100;
}
} // namespace eth
} // namespace dev
