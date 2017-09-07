#include "HttpNetwork.h"
#include <fstream>
#include <memory>
#include <string>

#include "../../Common/ConsoleLogger.h"
#include "../../Common/Define.h"
#include "../../Common/json.hpp"
#include "../../Common/ErrorCode.h"

#include "PacketInfo.h"

namespace FPNetwork
{
	// WARN :: HappyHttp ���̺귯���� �Լ� ���¸� �̷��� ������ ���� �־ ��� �Լ� �ݹ����� ����ϱⰡ ����������.
	void OnBegin(const happyhttp::Response* r, void* userdata)
	{
	}

	void OnData(const happyhttp::Response* r, void* userdata, const unsigned char* data, int n)
	{
		std::string &result = *(static_cast<std::string*>(userdata));
		result += std::string(reinterpret_cast<const char*>(data));
	}

	void OnComplete(const happyhttp::Response* r, void* userdata)
	{
	}

	using json = nlohmann::json;

	ErrorCode HttpNetwork::Init(ConsoleLogger * logger, PacketQueue * recvQueue)
	{
		_logger = logger;
		_recvQueue = recvQueue;

		if (LoadHttpConfig() != ErrorCode::None)
		{
			return ErrorCode::FailConfigLoad;
		}

		return ErrorCode::None;
	}

	std::string HttpNetwork::PostRequestToDBServer(std::string reqData, ApiEnum api)
	{
		const char* headers[] =
		{
			"Connection", "close",
			"Content-type", "application/json",
			"Accept", "text/plain",
			0
		};

		happyhttp::Connection conn(_config->_dbServerUrl.c_str(), _config->_dbServerPort);
		//happyhttp::Connection conn("10.73.39.93", 20000);
		// ��� ���� �ݹ� �Լ� ȣ��ÿ� ���ڷ� ������ ���.
		std::string result;
		conn.setcallbacks(OnBegin, OnData, OnComplete, static_cast<void*>(&result));
		conn.request("POST",
			_config->_dbServerApi[static_cast<int>(api)].c_str(),
			headers,
			(const unsigned char*)reqData.c_str(),
			strlen(reqData.c_str()));

		while (conn.outstanding())
			conn.pump();

		return result;
	}

	ErrorCode HttpNetwork::LoadHttpConfig()
	{
		std::ifstream configFile("C:\\HttpConfig.json");
		if (configFile.fail())
		{
			_logger->Write(LogType::LOG_ERROR, "%s | Config File Load Failed", __FUNCTION__);
			return ErrorCode::FailConfigLoad;
		}

		json configJson;
		configFile >> configJson;

		_config = std::make_unique<HttpConfig>();

		_config->_dbServerUrl = configJson["_dbServerUrl"].get<std::string>();
		_config->_dbServerPort = configJson["_dbServerPort"].get<int>();

		std::string api = "api";
		for (auto i = 1; i < static_cast<int>(ApiEnum::ApiMaxNum); ++i)
		{
			auto currentApi = api + std::to_string(i);
			_config->_dbServerApi[i] = configJson[currentApi].get<std::string>();
		}
	}
}