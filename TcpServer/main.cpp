#include <iostream>
#include <string>
#include <map>
#include <io.h>
#include "TcpServer.h"
#include "TcpConnection.h"
using namespace net;
using namespace std;

struct BuildConfig
{
	char appName[32];
	//������ͣ�0��������ʽ��125���������ԣ�888�������ڲ�
	int16_t buildType;
	char recommendID[32];
	char channel[32];
	bool isUpdateSVN;
	bool isEncrypt;
	bool isYQW;
	char gameSrc[256];
	char versionName[32];
	char versionCode[32];
};

struct Message
{
	int32_t		request;
	size_t		size;
};

void onMessage(const std::shared_ptr<TcpConnection>& conn, const void* data, size_t len);
void sendString(const std::shared_ptr<TcpConnection>& conn, const string& content);
void sendFile(const std::shared_ptr<TcpConnection>& conn, const string& path);

int main()
{
	boost::asio::io_service ioService;
	TcpServer server(ioService, 12345);
	server.setMessageCallback(onMessage);

	ioService.run();
}

void onMessage(const std::shared_ptr<TcpConnection>& conn, const void* data, size_t len)
{
	const BuildConfig* pConfig = reinterpret_cast<const BuildConfig*>(data);
	std::map<string, string> configs =
	{
		{ "appName",		pConfig->appName },
		{ "buildType",		to_string(pConfig->buildType) },
		{ "recommendID",	pConfig->recommendID },
		{ "channel",		pConfig->channel },
		{ "isUpdateSVN",	pConfig->isUpdateSVN ? "Y" : "N" },
		{ "isEncrypt",		pConfig->isEncrypt ? "Y" : "N" },
		{ "isYQW",			pConfig->isYQW ? "Y" : "N" },
		{ "gameSrc",		pConfig->gameSrc },
		{ "versionName",	pConfig->versionName },
		{ "versionCode",	pConfig->versionCode }
	};

	string content =
		"�������Ϊ:\n"
		"��Ϸ����: " + configs["appName"] + "\n"
		"�������: " + configs["buildType"] + "\n"
		"�ƹ�ID: " + configs["recommendID"] + "\n"
		"��������: " + configs["channel"] + "\n"
		"�Ƿ����SVN: " + configs["isUpdateSVN"] + "\n"
		"�Ƿ����: " + configs["isEncrypt"] + "\n"
		"�Ƿ�ʹ��һ��������: " + configs["isYQW"] + "\n"
		"svn����·��: " + configs["gameSrc"] + "\n"
		"�汾��: " + configs["versionName"] + "\n"
		"versionCode:" + configs["versionCode"] + "\n";

	sendString(conn, content);

	sendString(conn, "���ڴ���������ĵȴ�...\n");

	string cmd =
		"cd /D .\\build && python LuaBuildapks.py \"" +
		configs["appName"] + "\" \"" +
		configs["buildType"] + "\" \"" +
		configs["recommendID"] + "\" \"" +
		configs["channel"] + "\" \"" +
		configs["isUpdateSVN"] + "\" \"" +
		configs["isEncrypt"] + "\" \"" +
		configs["isYQW"] + "\" \"" + 
		configs["gameSrc"] + "\" \"" +
		configs["versionName"] + "\" \"" + 
		configs["versionCode"] + "\"";

	string path = "E:\\apk\\" + configs["appName"] + "\\" +
		configs["buildType"] + "\\" +
		configs["appName"] + "_" + configs["channel"] + ".zip";

	char delCmd[256];
	_snprintf(delCmd, sizeof(delCmd), "if exist %s del %s /q", path.data(), path.data());
	::system(delCmd);

	::system(cmd.c_str());

	if (::_access(path.c_str(), 0) == 0)
	{
		sendFile(conn, path);
	}
	else
	{
		sendString(conn, "���ʧ��, ���������Ƿ���ȷ��\n");
	}
}

void sendString(const std::shared_ptr<TcpConnection>& conn, const string& content)
{
	Message msg;
	msg.request = 10000;
	msg.size = content.size();

	if(!conn->send(&msg, sizeof(msg))) return;
	if(!conn->send(content.data(), content.size())) return;
}

void sendFile(const std::shared_ptr<TcpConnection>& conn, const string& path)
{
	Message msg;
	msg.request = 10001;
	
	FILE* fp = fopen(path.c_str(), "rb");
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		long len = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		msg.size = len;
		if(!conn->send(&msg, sizeof(msg))) return;

		char buf[128 * 1024];
		size_t nRead = fread(buf, 1, sizeof buf, fp);

		int hasSendBytes = 0;
		while (nRead > 0)
		{
			if(!conn->send(buf, nRead)) return;

			nRead = fread(buf, 1, sizeof buf, fp);
		}

		fclose(fp);
	}
	else
	{
		cerr << "failed to open " << path << endl;
	}
}