#include "DPwTestsHelper.h"
using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

namespace dev {
	namespace test {

BOOST_FIXTURE_TEST_SUITE(PowTestsSuite, TestOutputHelperFixture)
BOOST_AUTO_TEST_CASE(dtMakePowProducer)
{
	cout << "dtMakePowProducer" << endl;

	//g_logVerbosity = 14;
	//����������
	DposTestClient client;
	int num = 0;
	// pick an account
	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	auto& account = client.get_accounts()[0];

	//��ǰ�ִ�û��pow��������ǰ��һ��accountnameΪ��
	auto currentProducers = client.get_active_producers();
	for (auto pro : currentProducers)
	{
		if (types::AccountName(account.address) == types::AccountName(pro))
			num++;
	}
	BOOST_REQUIRE(num == 0);

	client.make_pow_producer(account, setPowTest::none);
	client.produce_blocks(config::TotalProducersPerRound);
	//���ִ�pow�󹤳ɹ�������������ִ�
	currentProducers = client.get_active_producers();
	num = 0;
	for (auto pro : currentProducers)
	{
		if (types::AccountName(account.address) == types::AccountName(pro))
			num++;
	}
	BOOST_REQUIRE(num == 1);

	//����һ�ֺ�pow������û����--�����ڼ������Ƿ���pow
	client.produce_blocks(config::TotalProducersPerRound);

	//���ִ�pow�󹤳ɹ�������������ִ�
	currentProducers = client.get_active_producers();
	num = 0;
	for (auto pro : currentProducers)
	{
		if (types::AccountName(account.address) == types::AccountName(pro))
			num++;
	}
	BOOST_REQUIRE(num == 0);
}

BOOST_AUTO_TEST_CASE(dtGetErrorSignature)
{
	cout << "dtGetErrorSignature" << endl;

	//g_logVerbosity = 14;
	//����������
	DposTestClient client;
	int num = 0;
	// pick an account
	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	auto& account = client.get_accounts()[0];

	//��ǰ�ִ�û��pow��������ǰ��һ��accountnameΪ��
	auto currentProducers = client.get_active_producers();
	for (auto pro : currentProducers)
	{
		if (types::AccountName(account.address) == types::AccountName(pro))
			num++;
	}
	BOOST_REQUIRE(num == 0);

	client.make_pow_producer(account, setPowTest::errorSignature);
	client.produce_blocks(config::TotalProducersPerRound);
	//���ִ�pow�󹤳ɹ�������������ִ�
	currentProducers = client.get_active_producers();
	for (auto pro : currentProducers)
	{
		if (types::AccountName(account.address) == types::AccountName(pro))
			num++;
	}
	BOOST_REQUIRE(num == 0);
}
BOOST_AUTO_TEST_CASE(dtGetErrorTarget)
{
	cout << "dtGetErrorTarget" << endl;

	//g_logVerbosity = 14;
	//����������
	DposTestClient client;
	int num = 0;
	// pick an account
	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	auto& account = client.get_accounts()[0];

	//��ǰ�ִ�û��pow��������ǰ��һ��accountnameΪ��
	auto currentProducers = client.get_active_producers();
	for (auto pro : currentProducers)
	{
		if (types::AccountName(account.address) == types::AccountName(pro))
			num++;
	}
	BOOST_REQUIRE(num == 0);

	client.make_pow_producer(account, setPowTest::errorTarget);
	client.produce_blocks(config::TotalProducersPerRound);
	//���ִ�pow�󹤳ɹ�������������ִ�
	currentProducers = client.get_active_producers();
	for (auto pro : currentProducers)
	{
		if (types::AccountName(account.address) == types::AccountName(pro))
			num++;
	}
	BOOST_REQUIRE(num == 0);
}

BOOST_AUTO_TEST_CASE(dtGetErrorBlockid)
{
	cout << "dtGetErrorBlockid" << endl;

	//g_logVerbosity = 14;
	//����������
	DposTestClient client;
	int num = 0;
	// pick an account
	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	auto& account = client.get_accounts()[0];

	//��ǰ�ִ�û��pow��������ǰ��һ��accountnameΪ��
	auto currentProducers = client.get_active_producers();
	for (auto pro : currentProducers)
	{
		if (types::AccountName(account.address) == types::AccountName(pro))
			num++;
	}
	BOOST_REQUIRE(num == 0);

	client.make_pow_producer(account, setPowTest::errorBlockid);
	client.produce_blocks(config::TotalProducersPerRound);
	//���ִ�pow�󹤳ɹ�������������ִ�
	currentProducers = client.get_active_producers();
	for (auto pro : currentProducers)
	{
		if (types::AccountName(account.address) == types::AccountName(pro))
			num++;
	}
	BOOST_REQUIRE(num == 0);

	ctrace << "account.balance = " << client.balance(AccountName(account.address));
	BOOST_REQUIRE((u256(1000000000000000000) - client.balance(AccountName(account.address))) >=0);
}
//ͬʱ��N���ڵ�������target
BOOST_AUTO_TEST_CASE(dtlowTarget)
{
	cout << "dtlowTarget" << endl;

	//g_logVerbosity = 14;
	//����������
	EthashCPUMiner::setNumInstances(1);
	DposTestClient client;
	int num = 0;
	// pick an account
	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	auto& accounts = client.get_accounts();

	//��ǰ�ִ�û��pow��������ǰ��һ��accountnameΪ��
	//auto currentProducers = client.get_active_producers();
	BOOST_REQUIRE(client.get_dpo_witnesses() == 0);

	for (auto i : accounts)
	{
		client.make_pow_producer(i, setPowTest::none);
	}

	client.produce_blocks(5);

	BOOST_REQUIRE(client.get_dpo_witnesses() == accounts.size());
	//�Ƚϱ�������������������Ƿ���ͬ
	BOOST_REQUIRE(client.get_ownpow_target() == client.get_pow_target());
}
//pow��������н��յ��µĿ飬ֹͣ����
BOOST_AUTO_TEST_CASE(dtHighTarget)
{
	cout << "dtHighTarget" << endl;

	//g_logVerbosity = 14;
	//����������
	DposTestClient client;
	int num = 0;
	// pick an account
	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	auto& accounts = client.get_accounts();

	//��ǰ�ִ�û��pow��������ǰ��һ��accountnameΪ��
	BOOST_REQUIRE(client.get_dpo_witnesses() == 0);

	for (auto i : accounts)
	{
		client.make_pow_producer(i, setPowTest::none);
	}

	client.produce_blocks(2);

	BOOST_REQUIRE(client.get_dpo_witnesses() == accounts.size());
	
}
//һ�������߼���������
BOOST_AUTO_TEST_CASE(dtGetTwoTarget)
{
	cout << "dtGetTwoTarget" << endl;

	//g_logVerbosity = 14;
	//����������
	DposTestClient client;
	int num = 0;
	// pick an account
	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	auto& account = client.get_accounts()[0];

	//��ǰ�ִ�û��pow��������ǰ��һ��accountnameΪ��
	auto currentProducers = client.get_active_producers();
	for (auto pro : currentProducers)
	{
		if (types::AccountName(account.address) == types::AccountName(pro))
			num++;
	}
	BOOST_REQUIRE(num == 0);

	for (int i = 1; i <= 2; i++)
	{
		client.make_pow_producer(account, setPowTest::none);
		client.produce_blocks(1);
	}
	client.produce_blocks(config::TotalProducersPerRound);
	//���ִ�pow�󹤳ɹ�������������ִ�
	currentProducers = client.get_active_producers();
	for (auto pro : currentProducers)
	{
		if (types::AccountName(account.address) == types::AccountName(pro))
			num++;
	}
	BOOST_REQUIRE(num == 1);
}

//�ж�pow��ѡ�е�ʱ�򣬶����м��ٶ�Ӧ��������
BOOST_AUTO_TEST_CASE(dtPowWitness)
{
	cout << "dtPowWitness" << endl;

	//g_logVerbosity = 14;
	//����������
	EthashCPUMiner::setNumInstances(1);
	DposTestClient client;
	int num = 0;
	// pick an account
	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	//�ȳ����ֿ����������
	client.produce_blocks(config::TotalProducersPerRound * 3);

	auto& accounts = client.get_accounts();

	//��ǰ�ִ�û��pow��������ǰ��һ��accountnameΪ��
	BOOST_REQUIRE(client.get_dpo_witnesses() == 0);

	for (auto i : accounts)
	{
		client.make_pow_producer(i, setPowTest::none);
	}

	client.produce_blocks(2);
	BOOST_REQUIRE(client.get_dpo_witnesses() == accounts.size());

	//��n��֮���ж�pow��Ա�Ƿ����
	int n = 2;
	for (auto i =1; i <= n;i++)
	{
		if (client.get_dpo_witnesses() > config::TotalProducersPerRound)
		{
			//��ȡ��ǰpow�����е���
			const auto& allProducerbyPow_start = client.get_allproducer_power();
			auto itr = allProducerbyPow_start.upper_bound(0);

			client.produce_blocks(config::TotalProducersPerRound);
			BOOST_REQUIRE(client.get_dpo_witnesses() == (accounts.size() - (config::POWProducersPerRound+1)*i));

			const auto& allProducerbyPow_end = client.get_allproducer_power();
			auto first = allProducerbyPow_end.upper_bound(0);
			BOOST_REQUIRE(itr != first);
		}
		else
		{
			client.produce_blocks(config::TotalProducersPerRound);
			BOOST_REQUIRE(client.get_dpo_witnesses() == (accounts.size() - config::POWProducersPerRound*i));
		}
		
	}
}

//Pow�����ʱ������¿飬
BOOST_AUTO_TEST_CASE(dtPowingaddBlock)
{
	cout << "dtPowingaddBlock" << endl;

	//g_logVerbosity = 14;
	//����������
	DposTestClient client;
	int num = 0;
	// pick an account
	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	auto& account = client.get_accounts()[0];

	//��ǰ�ִ�û��pow��������ǰ��һ��accountnameΪ��
	auto currentProducers = client.get_active_producers();
	for (auto pro : currentProducers)
	{
		if (types::AccountName(account.address) == types::AccountName(pro))
			num++;
	}
	BOOST_REQUIRE(num == 0);

	client.add_new_Work(account);
	client.produce_blocks(config::TotalProducersPerRound);
	//���ִ�pow�󹤳ɹ�������������ִ�
	currentProducers = client.get_active_producers();
	for (auto pro : currentProducers)
	{
		if (types::AccountName(account.address) == types::AccountName(pro))
			num++;
	}
	BOOST_REQUIRE(num == 1);

}

BOOST_AUTO_TEST_SUITE_END()



BOOST_FIXTURE_TEST_SUITE(BlockTestsSuite, TestOutputHelperFixture)
/*�����ڵ�test����Ҫ�Ѵ����ڵĿ�߶ȸĳ�84*/
BOOST_AUTO_TEST_CASE(dtMoreDposProducer)
{
	cout << "dtMoreDposProducer" << endl;

	//g_logVerbosity = 14;
	//����������
	DposTestClient client;
	int num = 0;
	// pick account
	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	auto& accounts = client.get_accounts();

	//��ǰû��DPOS������
	auto VoteProducers = client.get_all_producers();
	BOOST_REQUIRE(VoteProducers.size() == 0);

	//ע��DPOS������
	for (auto &i : accounts)
	{
		client.make_producer(i);
		client.mortgage_eth(i, 500000000000000000);
	}
	client.produce_blocks();
	auto all_votes = client.get_votes();
	BOOST_REQUIRE_EQUAL(all_votes[types::AccountName(accounts[0].address)].getHoldVoteNumber(), 50);
	for (auto &i : accounts)
	{
		client.approve_producer(i, i, 30);
	}
	//�����ڳ���
	client.produce_blocks(config::TotalProducersPerRound-1);
	all_votes = client.get_votes();
	BOOST_REQUIRE_EQUAL(all_votes[types::AccountName(accounts[0].address)].getReceivedVotedNumber(), 30);

	//��ǰȫ������DPOS�����߳���
	VoteProducers = client.get_all_producers();
	BOOST_REQUIRE(VoteProducers.size() == accounts.size());

	auto currentProducers = client.get_active_producers();
	std::vector<Address> initProducers = client.getGenesisAccount();
	for (auto i : initProducers)
	{
		for (auto pro : currentProducers)
		{
			if (types::AccountName(i) == types::AccountName(pro))
				num++;
		}
	}
	BOOST_REQUIRE_EQUAL(num,0);
}

BOOST_AUTO_TEST_CASE(dtLittleDposProducer)
{
	cout << "dtLittleDposProducer" << endl;

	//g_logVerbosity = 14;
	//����������
	DposTestClient client;
	int num = 0;
	// pick account
	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	auto& accounts = client.get_accounts();

	//��ǰû��DPOS������
	auto VoteProducers = client.get_all_producers();
	ctrace << "currentProducers : " << VoteProducers.size() ;
	BOOST_REQUIRE(VoteProducers.size() == 0);

	//ע��DPOS������
	int producerNum = 15;
	for (auto i =0;i <producerNum;i++)
	{
		client.make_producer(accounts[i]);
	}
	//�����ڳ���
	client.produce_blocks(config::TotalProducersPerRound);

	//��ǰȫ������DPOS�����߳���
	VoteProducers = client.get_all_producers();
	BOOST_REQUIRE(VoteProducers.size() == producerNum);

	auto currentProducers = client.get_active_producers();
	std::vector<Address> initProducers = client.getGenesisAccount();
	for (auto i : initProducers)
	{
		for (auto pro : currentProducers)
		{
			if (types::AccountName(i) == types::AccountName(pro))
				num++;
		}
	}
	BOOST_REQUIRE(num != 0);
}

BOOST_AUTO_TEST_CASE(dtMakePowProducer)
{
	cout << "dtMakePowProducer" << endl;

	//g_logVerbosity = 14;
	//����������
	DposTestClient client;
	int num = 0;
	// pick an account
	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	auto& account = client.get_accounts()[0];

	//��ǰ�ִ�û��pow��������ǰ��һ��accountnameΪ��
	auto currentProducers = client.get_active_producers();
	for (auto pro : currentProducers)
	{
		if (types::AccountName(account.address) == types::AccountName(pro))
			num++;
	}
	BOOST_REQUIRE(num == 0);

	client.make_pow_producer(account, setPowTest::none);
	client.produce_blocks(config::TotalProducersPerRound);
	//���ִ�pow�󹤳ɹ�������������ִ�
	currentProducers = client.get_active_producers();
	num = 0;
	for (auto pro : currentProducers)
	{
		if (types::AccountName(account.address) == types::AccountName(pro))
			num++;
	}
	BOOST_REQUIRE(num == 1);

	//����һ�ֺ�pow������û����--�����ڼ������Ƿ���pow
	client.produce_pow_blocks(AccountName(account.address), config::TotalProducersPerRound);

	//���ִ�pow�󹤳ɹ�������������ִ�
	currentProducers = client.get_active_producers();
	num = 0;
	for (auto pro : currentProducers)
	{
		if (types::AccountName(account.address) == types::AccountName(pro))
			num++;
	}
	BOOST_REQUIRE(num == 0);
}

BOOST_AUTO_TEST_CASE(dtMakeMorePowProducer)
{
	cout << "dtMakeMorePowProducer" << endl;

	//g_logVerbosity = 14;
	//����������
	EthashCPUMiner::setNumInstances(1);
	DposTestClient client;
	int num = 0;
	// pick an account
	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	auto& accounts = client.get_accounts();

	//��ǰ�ִ�û��pow��������ǰ��һ��accountnameΪ��
	auto currentProducers = client.get_active_producers();
	for (auto pro : currentProducers)
	{
		for(auto i : accounts)
		   if (types::AccountName(i.address) == types::AccountName(pro))
			   num++;
	}
	BOOST_REQUIRE(num == 0);

	//ע��Pow������
	for (auto i : accounts)
	{
		client.make_pow_producer(i, setPowTest::none);
	}
	//�����ڳ���
	client.produce_blocks(config::TotalProducersPerRound-1);

	if (client.get_dpo_witnesses() > config::TotalProducersPerRound)
	{
		client.produce_blocks();
		//���ִ�pow�󹤳ɹ�������������ִ�
		currentProducers = client.get_active_producers();
		num = 0;
		for (auto pro : currentProducers)
		{
			for (auto i : accounts)
				if (types::AccountName(i.address) == types::AccountName(pro))
					num++;
		}
		BOOST_REQUIRE(num == currentProducers.size());
		//�ڶ��ֳ��飬Ӧ����ʣ��ĸ�pow������
		client.produce_blocks(config::TotalProducersPerRound);
		//���ִ�pow�󹤳ɹ�������������ִ�
		currentProducers = client.get_active_producers();
		num = 0;
		for (auto pro : currentProducers)
		{
			for (auto i : accounts)
				if (types::AccountName(i.address) == types::AccountName(pro))
					num++;
		}
		//��ȡ��ǰpow�����е���
		BOOST_REQUIRE_EQUAL(num, accounts.size() - config::TotalProducersPerRound -1);
	}
	else
	{
		client.produce_blocks();
		//���ִ�pow�󹤳ɹ�������������ִ�
		currentProducers = client.get_active_producers();
		num = 0;
		for (auto pro : currentProducers)
		{
			for (auto i : accounts)
				if (types::AccountName(i.address) == types::AccountName(pro))
					num++;
		}
		BOOST_REQUIRE(num == currentProducers.size());
		//�ڶ��ֳ��飬Ӧ����ʣ��ĸ�pow������
		client.produce_blocks(config::TotalProducersPerRound);

		//���ִ�pow�󹤳ɹ�������������ִ�
		currentProducers = client.get_active_producers();
		num = 0;
		for (auto pro : currentProducers)
		{
			for (auto i : accounts)
				if (types::AccountName(i.address) == types::AccountName(pro))
					num++;
		}
		client.produce_blocks(config::TotalProducersPerRound);
		BOOST_REQUIRE_EQUAL(num, accounts.size() - currentProducers.size());
	}
	
}

/*ע�⣺����ʱ��ͨ������makeProducerCount����������ע���pow����*/
BOOST_AUTO_TEST_CASE(dtMakeLittlePowProducer)
{
	cout << "dtMakeLittlePowProducer" << endl;

	//g_logVerbosity = 14;
	//����������
	EthashCPUMiner::setNumInstances(1);
	DposTestClient client;
	int num = 0;
	// pick an account
	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	auto& accounts = client.get_accounts();

	//��ǰ�ִ�û��pow��������ǰ��һ��accountnameΪ��
	auto currentProducers = client.get_active_producers();
	for (auto pro : currentProducers)
	{
		for (auto i : accounts)
			if (types::AccountName(i.address) == types::AccountName(pro))
				num++;
	}
	BOOST_REQUIRE(num == 0);

	//ע��Pow������
	int makeProducerCount = 10;
	for (auto i =0;i < makeProducerCount;i++)
	{
		client.make_pow_producer(accounts[i], setPowTest::none);
	}
	//�����ڳ���
	client.produce_blocks(config::TotalProducersPerRound);

	//���ִ�pow�󹤳ɹ�������������ִ�
	currentProducers = client.get_active_producers();
	num = 0;
	for (auto pro : currentProducers)
	{
		for (auto i : accounts)
			if (types::AccountName(i.address) == types::AccountName(pro))
				num++;
	}

	BOOST_REQUIRE_EQUAL(num , makeProducerCount);

}

/*������->�ȶ���*/
//���Ե�ʱ����Ҫ��̬����powע��ĸ���
BOOST_AUTO_TEST_CASE(dtCheckPowProducer)
{
	cout << "dtCheckPowProducer" << endl;

	//g_logVerbosity = 14;
	//����������
	EthashCPUMiner::setNumInstances(1);
	DposTestClient client;
	int num = 0;
	// pick an account
	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	auto& accounts = client.get_accounts();

	//��ǰ�ִ�û��pow��������ǰ��һ��accountnameΪ��
	auto currentProducers = client.get_active_producers();
	for (auto pro : currentProducers)
	{
		for (auto i : accounts)
			if (types::AccountName(i.address) == types::AccountName(pro))
				num++;
	}
	BOOST_REQUIRE(num == 0);
	//�ȳ����֣�ʹ��߶ȴﵽ63
	client.produce_blocks(config::TotalProducersPerRound*2);
	//ע��Pow������
	for (auto i : accounts)
	{
		client.make_pow_producer(i, setPowTest::none);
	}
	//�����ڳ���
	client.produce_blocks(config::TotalProducersPerRound);

	//���ִ�pow�󹤳ɹ�������������ִ�
	currentProducers = client.get_active_producers();
	num = 0;
	for (auto pro : currentProducers)
	{
		for (auto i : accounts)
			if (types::AccountName(i.address) == types::AccountName(pro))
				num++;
	}

	BOOST_REQUIRE(num == currentProducers.size());

	//�ȶ��ڳ��飬Ӧ����ʣ��ĸ�pow������
	client.produce_blocks(config::TotalProducersPerRound);

	//���ִ�pow�󹤳ɹ�������������ִ�
	currentProducers = client.get_active_producers();
	num = 0;
	for (auto pro : currentProducers)
	{
		for (auto i : accounts)
			if (types::AccountName(i.address) == types::AccountName(pro))
				num++;
	}
	//1)pow���������£���̬������
	BOOST_REQUIRE_EQUAL(num , config::POWProducersPerRound);
	//2)pow����������
	//BOOST_REQUIRE(num == accounts.size()-config::TotalProducersPerRound);
	//BOOST_REQUIRE(num > 0);

}

BOOST_AUTO_TEST_SUITE_END()




BOOST_FIXTURE_TEST_SUITE(StableBlockTestsSuite, TestOutputHelperFixture)

//��DPOS�ڵ�
BOOST_AUTO_TEST_CASE(dtCheckPowProducers)
{
	cout << "dtCheckPowProducers" << endl;

	//g_logVerbosity = 14;
	//����������
	EthashCPUMiner::setNumInstances(1);
	DposTestClient client;
	int num = 0;
	// pick an account
	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	auto& accounts = client.get_accounts();

	//��ǰ�ִ�û��pow��������ǰ��һ��accountnameΪ��
	auto currentProducers = client.get_active_producers();
	for (auto pro : currentProducers)
	{
		for (auto i : accounts)
			if (types::AccountName(i.address) == types::AccountName(pro))
				num++;
	}
	BOOST_REQUIRE_EQUAL(num, 0);
	//�ȳ����ֿ����������
	client.produce_blocks(config::TotalProducersPerRound*3);
	//ע��Pow������
	for (auto i : accounts)
	{
		client.make_pow_producer(i, setPowTest::none);
	}
	//
	client.produce_blocks(config::TotalProducersPerRound);

	//���ִ�pow�󹤳ɹ�������������ִ�
	currentProducers = client.get_active_producers();
	num = 0;
	for (auto pro : currentProducers)
	{
		for (auto i : accounts)
			if (types::AccountName(i.address) == types::AccountName(pro))
				num++;
	}
	//1)pow���������£���̬������
	BOOST_REQUIRE_EQUAL(num ,config::POWProducersPerRound);
	//2)pow����������
	//BOOST_REQUIRE(num == accounts.size());
	//BOOST_REQUIRE(num > 0);

}
//��POW�ڵ�
BOOST_AUTO_TEST_CASE(dtNoPowProducers)
{
	cout << "dtNoPowProducers" << endl;

	//g_logVerbosity = 14;
	//����������
	DposTestClient client;
	int num = 0;
	// pick an account
	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	auto& accounts = client.get_accounts();

	//��ǰ�ִ�û��pow��������ǰ��һ��accountnameΪ��
	auto currentProducers = client.get_active_producers();
	for (auto pro : currentProducers)
	{
		for (auto i : accounts)
			if (types::AccountName(i.address) == types::AccountName(pro))
				num++;
	}
	BOOST_REQUIRE(num == 0);

	//�����ڳ���
	client.produce_blocks(config::TotalProducersPerRound);

	//���ִ�pow�󹤳ɹ�������������ִ�
	currentProducers = client.get_active_producers();
	num = 0;
	for (auto pro : currentProducers)
	{
		for (auto i : accounts)
			if (types::AccountName(i.address) == types::AccountName(pro))
				num++;
	}
	//1)pow���������£���̬������
	//BOOST_REQUIRE(num == config::POWProducersPerRound);
	//2)pow����������
	BOOST_REQUIRE(num == 0);

}
//��POW�ڵ㡢DPOS�ڵ����
BOOST_AUTO_TEST_CASE(dtEnoughDposProducers)
{
	cout << "dtEnoughDposProducers" << endl;

	//g_logVerbosity = 14;
	//����������
	DposTestClient client;
	int num = 0;
	// pick an account
	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	auto& accounts = client.get_accounts();

	//��ǰ�ִ�û��pow��������ǰ��һ��accountnameΪ��
	auto currentProducers = client.get_active_producers();
	for (auto pro : currentProducers)
	{
		if (types::AccountName() != types::AccountName(pro))
			num++;
	}
	BOOST_REQUIRE(num == client.getGenesisAccount().size());

	for (auto i = 0; i <= 10; i++)
	{
		client.make_producer(accounts[i]);
	}

	//�����ڳ���
	client.produce_blocks(config::TotalProducersPerRound);

	//���ִ�
	currentProducers = client.get_active_producers();
	num = 0;
	for (auto pro : currentProducers)
	{
		if (types::AccountName() != types::AccountName(pro))
			num++;
	}

	BOOST_REQUIRE(num == config::DPOSProducersPerRound);

}

BOOST_AUTO_TEST_CASE(dtRaceSpeedTest)
{
	cout << "dtRaceSpeedTest" << endl;

	//g_logVerbosity = 14;
	//����������
	EthashCPUMiner::setNumInstances(1);
	DposTestClient client;

	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	//�ȳ����ֿ����������
	client.produce_blocks(config::TotalProducersPerRound * 3);

	auto& accounts = client.get_accounts();

	//1.ע��19��������
	for (auto i = 0; i < 19; i++)
	{
		client.make_producer(accounts[i]);
	}
	client.produce_blocks();
	for (auto i = 19; i < 23; i++)
	{
		client.make_pow_producer(accounts[i], setPowTest::none);
	}
	client.produce_blocks();
	//2.ѡ��ǰ16��������,��Ѻ��ÿ������һ����
	for (auto i = 0; i < 21; i++)
	//for (auto i = 0; i < 19; i++)
	{
		client.mortgage_eth(accounts[i], 600000000000000000);	
	}
	client.produce_blocks();
	//3.ͶƱ������
	for (auto i = 0; i < config::DPOSVotedProducersPerRound; i++)
	{	
		client.approve_producer(accounts[i], accounts[i], 60);
	}
	client.produce_blocks();
	//4.ѡ���������ܵ�������
	for (auto i = config::DPOSVotedProducersPerRound,j =20; i < config::DPOSVotedProducersPerRound + 3; i++,j += 10)
	{
		client.approve_producer(accounts[i], accounts[i], j);
	}
	//��������ʣ�����
	client.produce_blocks(17);

	//��3-9�ֳ��飬У�������������ܵ������߳���ĸ������Ƿ�Ϊ2��3��4
	std::map<AccountName, int> account_block;
	for (auto run = 0; run < 9; run++)
	{
		for (auto i = 19; i < 23; i++)
		{
			client.make_pow_producer(accounts[i], setPowTest::none);
		}
		client.produce_blocks_Number(config::TotalProducersPerRound, account_block);

	}

	BOOST_REQUIRE_EQUAL(account_block[AccountName(accounts[16].address)] , 2);
	BOOST_REQUIRE_EQUAL(account_block[AccountName(accounts[17].address)] , 3);
	BOOST_REQUIRE_EQUAL(account_block[AccountName(accounts[18].address)] , 4);
}

BOOST_AUTO_TEST_CASE(dtVoteChangeTest)
{
	cout << "dtVoteChangeTest" << endl;

	//g_logVerbosity = 13;
	//����������
	EthashCPUMiner::setNumInstances(1);
	DposTestClient client;
	BOOST_REQUIRE(client.get_accounts().size() >= 1);
	//�ȳ����ֿ����������
	client.produce_blocks(config::TotalProducersPerRound * 3);

	auto& accounts = client.get_accounts();

	//1.ע��19��������
	for (auto i = 0; i < 19; i++)
	{
		client.make_producer(accounts[i]);
	}
	client.produce_blocks();
	//ע��pow������
	for (auto i = 19; i < 23; i++)
	{
		client.make_pow_producer(accounts[i], setPowTest::none);
	}
	client.produce_blocks();
	//2.ѡ��ǰ16��������,��Ѻ��ÿ������һ����
	for (auto i = 0; i < 19; i++)
	{
		client.mortgage_eth(accounts[i], 600000000000000000);
	}
	client.produce_blocks();
	//3.ͶƱ������
	for (auto i = 0; i < config::DPOSVotedProducersPerRound; i++)
	{
		client.approve_producer(accounts[i], accounts[i], 60);
	}
	//4.ѡ���������ܵ�������
	for (auto i = config::DPOSVotedProducersPerRound, j = 20; i < config::DPOSVotedProducersPerRound + 3; i++, j += 10)
	{
		client.approve_producer(accounts[i], accounts[i], j);
	}
	//5.��������ʣ�����
	client.produce_blocks(18);

	//6.��3-9�ֳ��飬У��ÿ������������Ӧ�Ŀ����Ƿ���ȷ
	std::map<AccountName, int> account_block;
	for (auto run = 0; run < 9; run++)
	{
		for (auto i = 19; i < 23; i++)
		{
			client.make_pow_producer(accounts[i], setPowTest::none);
		}
		client.produce_blocks_Number(config::TotalProducersPerRound, account_block);

	}
	for (auto i = 0; i < 16; i++)
	{
		BOOST_CHECK_MESSAGE(account_block[AccountName(accounts[0].address)], 9);
	}

	BOOST_REQUIRE_EQUAL(account_block[AccountName(accounts[16].address)] , 2);
	BOOST_REQUIRE_EQUAL(account_block[AccountName(accounts[17].address)] , 3);
	BOOST_REQUIRE_EQUAL(account_block[AccountName(accounts[18].address)] , 4);

	//7.���������ܵ�����������ͶƱ��ϢʹͶƱ���ڵ���60Ʊ
    client.approve_producer(accounts[16], accounts[16],40);
	client.produce_blocks(config::TotalProducersPerRound);

	//8.�����ֿ�
	for (auto run = 0; run < 3; run++)
	{
		for (auto i = 19; i < 23; i++)
		{
			client.make_pow_producer(accounts[i], setPowTest::none);
		}
		client.produce_blocks_Number(config::TotalProducersPerRound, account_block);

	}
	for (auto i = 0; i < 16; i++)
	{
		BOOST_CHECK_MESSAGE(account_block[AccountName(accounts[0].address)], 12);
	}
	BOOST_REQUIRE(account_block[AccountName(accounts[16].address)] == 5);
	ctrace << "17 : " <<account_block[AccountName(accounts[17].address)];
	ctrace << "18 : " << account_block[AccountName(accounts[18].address)];
	BOOST_REQUIRE_MESSAGE(account_block[AccountName(accounts[17].address)], 4);
	BOOST_REQUIRE_MESSAGE(account_block[AccountName(accounts[18].address)],4);
}

BOOST_AUTO_TEST_CASE(dtMakeBlockETHTest)
{
	cout << "dtMakeBlockETHTest" << endl;

	//1.����һ��client
	//g_logVerbosity = 13;
	//����������
	DposTestClient client;
	BOOST_REQUIRE(client.get_accounts().size() >= 1);

	//2.ע��һ��dpos�����ߣ���ȡbalance
	auto& account = client.get_accounts()[0];
	client.make_producer(account);

	//3.��һ�ֳ���
	client.produce_blocks(config::TotalProducersPerRound);
	u256 start_balance = client.balance(AccountName(account.address));

	//4.�ڶ��ֳ���
	std::map<AccountName, int> account_block;
	client.produce_blocks_Number(config::TotalProducersPerRound*10, account_block);

	//5.��ȡbalance�ͳ�����
	u256 end_balance = client.balance(AccountName(account.address));
	int blockNums = account_block[AccountName(account.address)];

	//6.�Ƚ�balance�ͳ�����
	BOOST_REQUIRE((end_balance - start_balance) / blockNums == (u256)5000000000000000000 );

}

BOOST_AUTO_TEST_SUITE_END()




}
}



