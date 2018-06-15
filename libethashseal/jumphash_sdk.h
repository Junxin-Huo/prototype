#pragma once

#include <libdevcore/FixedHash.h>
#include "ETIProofOfWork.h"
#include "libdevcore/Worker.h"

using namespace dev;


class jump_hash : public Worker
{

	struct work_package {
		h256 blockId;
		Secret privateKey;
		types::AccountName workerAccount;
		dev::h256 target;
		uint64_t nonce;
	};

	struct solution {
		uint64_t nonce;
		h256 workinput;
		h520 signature;
		h256 work;
	};

public:
	jump_hash();
	~jump_hash();

	/*
	*��������
	*����ֵ����
	*��Ϊ����ʼ��SDK״̬����Ӳ����س�ʼ������
	*����ʱ������ȫ�ڵ��ڳ�ʼ��ʱ���ã���ȫ�ڵ�����������ֻ����һ��
	*/
	void init_jumphash_sdk();

	/*
	��������
	*����ֵ����
	*��Ϊ������SDK������ڴ漰����Ӳ��������ٶ���
	*����ʱ������ȫ�ڵ����˳�ʱ���ã���ȫ�ڵ�����������ֻ����һ��
	*/
	void destroy_jumphash_sdk();

	/*
	*������ 
	*block_id ��ǰ��ID���ֳ�256λ
	*start_nonce ��ʼnonce��ÿ��ö��hashʱ�ڴ˻����Ͻ��е��ӣ��ֳ�64λ
	*worker_priv_key ��˽Կ����Ҫ���뵽�����㷨�У��ֳ�256λ
	*target Ŀ���Ѷȣ������������С�ڴ�ֵ���ֳ�256λ
	*����ֵ����
	*��Ϊ�����������Ҫ�������Ժ���������׼��
	*����ʱ�����ڿ�ȫ�ڵ�ÿ�νӵ��¿�ʱ����
	*/
	void prepare_calc_pow(const h256& block_id, const uint64_t start_nonce, const Secret& worker_priv_key, const h256& target);

	/*
	*��������
	*����ֵ����
	*��Ϊ������Ӳ����ʼȫ�����⣬�����㷨��������ġ������㷨��С��
	*����ʱ������Ҫ��prepare_calc_pow֮�����
	*/
	void start_calc_pow();

	/*
	*��������
	*����ֵ��bool ֪ͨȫ�ڵ�ͻ��˴�ʱ�Ƿ�����������Ѷ�Ҫ�����⣬true��ʾ�������Ŀ��false��ʾδ�����Ŀ
	*��Ϊ����sdk��ͻ��˻㱨�Ƿ��ѽ������
	*����ʱ������begin_calc_pow����ã�������ѯ�鿴�Ƿ��ѽ������
	*/
	bool is_calc_finished();

	/*
	*������
	*nonce: ���������õ�nonce
	*workinput: �������workinput
	*signature: ʹ�ÿ�˽Կǩ����workinput
	*work: ���յ����
	*����ֵ��bool
	*true ��ȡ�ɹ���˵������ʱ���Ѿ���������Ѷȵ����
	*false ��ȡʧ�ܣ���δ��������Ѷȵ����
	*��Ϊ��SDK��Ҫ������⼰����������е�һЩ�м���
	*����ʱ�����ڿͻ��˷���SDK��������ʱ
	*/
	bool query_pow_result(uint64_t& nonce, h256& workinput, h520& signature, h256& work);

	/*
	*��������
	*����ֵ����
	*��Ϊ����ֹ����������
	*����ʱ�������ڳ�������Ŀʱ���¿������δ�����һ���⣩����
	*/
	void stop_calc_pow();

	/*
	*������
	*worker_pub_key�� �󹤹�Կ
	*block_id: ������ڵĿ�ID
	*nonce: ���������õ�nonce
	*workinput: ���������õ�workinput
	*work: ���յ����
	*signature: �ÿ�˽Կǩ����workinput
	*target: ��ĿҪ������Ѷ�
	*����ֵ��bool
	*true ͨ����֤
	*false δͨ����֤
	*��Ϊ���㷨�μ����桰��֤�㷨��С��
	*����ʱ��������ȫ�ڵ�ͻ��˽ӵ����˵�pow���ʱ���ã�����������֤
	*/
	bool validate_pow_result(const h512& worker_pub_key, const h256& block_id, const uint64_t nonce, const h256& workinput, const h256& work, const h520& signature, const h256& target);

private:

	void workLoop() override;

	work_package const& work() const { Guard l(x_work); return m_work; }

	work_package m_work;
	mutable Mutex x_work;

	solution m_solution;
	bool m_solution_is_ready;
	mutable Mutex x_solution;

};

