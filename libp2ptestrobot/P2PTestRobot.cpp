#include "P2PTestRobot.hpp"
#include <string>

#include <utils/json_spirit/json_spirit_value.h>
#include <utils/json_spirit/json_spirit_reader_template.h>
#include <utils/json_spirit/json_spirit_writer_template.h>
#include <libweb3jsonrpc/JsonHelper.h>

#include <libethereum/CommonNet.h>
#include <libp2p/common.h>

using namespace P2PTest;
using namespace dev;
using namespace dev::eth;
using namespace dev::p2p;

P2PTestRobot::P2PTestRobot()
{
	m_idOffset = UserPacket;
}

P2PTestRobot::~P2PTestRobot()
{

}

void P2PTestRobot::requestBlockHeaders(dev::h256 const& _startHash, unsigned _count, unsigned _skip, bool _reverse)
{
	RLPStream s;
	//s.appendRaw(bytes(1, GetBlockHeadersPacket + 4)).appendList(_args) << _startHash << _count << _skip << (_reverse ? 1 : 0);
	//prep(s, GetBlockHeadersPacket, 4) << _startHash << _count << _skip << (_reverse ? 1 : 0);
	//clog(NetMessageDetail) << "Requesting " << _count << " block headers starting from " << _startHash << (_reverse ? " in reverse" : "");
	ctrace << "Requesting " << _count << " block headers starting from " << _startHash << (_reverse ? " in reverse" : "");
	//sealAndSend(s);
}

RLPStream& P2PTestRobot::prep(RLPStream& _s, unsigned _id, unsigned _args)
{
	return _s.appendRaw(bytes(1, _id + m_idOffset)).appendList(_args);
}

void P2PTestRobot::sealAndSend(RLPStream& _s)
{
	bytes b;
	_s.swapOut(b);
	sendToHost(move(b));
}

void P2PTestRobot::sendToHost(bytes& _s)
{

}


void P2PTestRobot::loadConfig()
{
	std::string configPath = "P2PTestRobotConfig.json";
	boost::filesystem::path _path(configPath);
	if (_path.is_relative())
	{
		std::string filePath(boost::filesystem::current_path().string());
		_path = boost::filesystem::path(filePath + "/" + configPath);
	}
	std::string s = dev::contentsString(_path);
	if (s.size() == 0)
	{
		BOOST_THROW_EXCEPTION(std::runtime_error("Config file doesn't exist!"));
	}
	json_spirit::mValue v;
	json_spirit::read_string(s, v);
	json_spirit::mObject& json_config = v.get_obj();

	if (!json_config.count("attackType") || !json_config.count("interval"))
	{
		BOOST_THROW_EXCEPTION(std::runtime_error("Invalid config file!"));
	}
}


void requestBlockHeaders(dev::h256 const& _startHash, unsigned _count, unsigned _skip, bool _reverse)
{

}