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
	g_logVerbosity = 14;
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
	for (auto pro : currentProducers)
	{
		if (types::AccountName(account.address) == types::AccountName(pro))
			num++;
	}
	BOOST_REQUIRE(num == 1);
}

BOOST_AUTO_TEST_CASE(dtGetErrorSignature)
{
	g_logVerbosity = 14;
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
	g_logVerbosity = 14;
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
	g_logVerbosity = 14;
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

	std::cout << "account.balance = " << client.balance(AccountName(account.address)) << std::endl;
	BOOST_REQUIRE((u256(1000000000000000000) - client.balance(AccountName(account.address))) >=0);
}
//ͬʱ��N���ڵ�������target
BOOST_AUTO_TEST_CASE(dtlowTarget)
{
	g_logVerbosity = 14;
	//����������
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
	g_logVerbosity = 14;
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
	g_logVerbosity = 14;
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
	g_logVerbosity = 14;
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

	//��n��֮���ж�pow��Ա�Ƿ����
	int n = 2;
	for (auto i =1; i <= n;i++)
	{
		client.produce_blocks(config::TotalProducersPerRound);
		BOOST_REQUIRE(client.get_dpo_witnesses() == (accounts.size() - config::POWProducersPerRound*i));
	}
}

//Pow�����ʱ������¿飬
BOOST_AUTO_TEST_CASE(dtPowingaddBlock)
{
	g_logVerbosity = 14;
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
}
}



