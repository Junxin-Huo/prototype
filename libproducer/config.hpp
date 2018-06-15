#pragma once
//#include <boost/multiprecision/cpp_int.hpp>
#include <libdevcore/Common.h>

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
	  

	//ÿ��DPOS Worker����
	const static int DPOSProducersPerRound = 4;
	//ÿ�ֿ�ѡƱ�ȶ�����DPOS��֤������
	const static int DPOSVotedProducersPerRound = 3;
	//ÿ���������ܵļ�֤������
	const static int DPOSRunnerupProducersPerRound = 1;
	//ÿ��POW ��֤������
	const static int POWProducersPerRound = 1;
	//ÿ�ּ�֤������
	const static int TotalProducersPerRound = 5;


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
