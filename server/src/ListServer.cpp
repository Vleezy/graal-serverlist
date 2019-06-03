#include "ListServer.h"
#include "TPlayer.h"
#include "TServer.h"

#include "MySQLBackend.h"

ListServer::ListServer(const std::string& homePath)
	: _initialized(false), _homePath(homePath), _dataStore(nullptr)
{
	_clientLog.setFilename(_homePath + "clientlog.txt");
	_serverLog.setFilename(_homePath + "serverlog.txt");
}

ListServer::~ListServer()
{
	Cleanup();
}

InitializeError ListServer::Initialize()
{
	// Already initialized
	if (_initialized)
		return InitializeError::None;

	// Load settings
	_settings.setSeparator("=");
	if (!_settings.loadFile(_homePath + "settings.ini"))
		return InitializeError::InvalidSettings;

	// Load server types
	//_serverTypes = CString::loadToken("servertypes.txt", "\n", true);

	// Bind the server socket
	CString serverInterface = _settings.getStr("gserverInterface");

	if (serverInterface == "AUTO")
		serverInterface.clear();
	_serverSock.setType(SOCKET_TYPE_SERVER);
	_serverSock.setProtocol(SOCKET_PROTOCOL_TCP);
	_serverSock.setDescription("serverSock");

	if (_serverSock.init((serverInterface.isEmpty() ? 0 : serverInterface.text()), _settings.getStr("gserverPort").text()))
		return InitializeError::ServerSock_Init;
	if (_serverSock.connect())
		return InitializeError::ServerSock_Listen;

	// Bind the player socket
	CString clientInterface = _settings.getStr("clientInterface");
	if (clientInterface == "AUTO")
		clientInterface.clear();

	_playerSock.setType(SOCKET_TYPE_SERVER);
	_playerSock.setProtocol(SOCKET_PROTOCOL_TCP);
	_playerSock.setDescription("playerSock");

	if (_playerSock.init((clientInterface.isEmpty() ? 0 : clientInterface.text()), _settings.getStr("clientPort").text()))
		return InitializeError::PlayerSock_Init;
	if (_playerSock.connect())
		return InitializeError::PlayerSock_Listen;

	// TODO(joey): Old player sock?? Unsure what its for, leaving out for now



	// TODO(joey): Create different data backends (likely do a text-based one as well)
	_dataStore = new MySQLBackend(_settings.getStr("server").text(), _settings.getInt("port"), _settings.getStr("sockfile").text(),
		_settings.getStr("user").text(), _settings.getStr("password").text(), _settings.getStr("database").text());

	// Connect to backend
	if (_dataStore->Initialize())
		return InitializeError::Backend_Error;

	_initialized = true;
	return InitializeError::None;
}

void ListServer::Cleanup()
{
	if (!_initialized)
		return;

	// Delete the players
	for (auto it = _playerConnections.begin(); it != _playerConnections.end(); ++it)
		delete *it;
	_playerConnections.clear();

	// Delete the servers
	for (auto it = _serverConnections.begin(); it != _serverConnections.end(); ++it)
		delete *it;
	_serverConnections.clear();

	// Disconnect sockets
	_serverSock.disconnect();
	_playerSock.disconnect();
}

bool ListServer::Main()
{
	auto currentTimer = std::chrono::high_resolution_clock::now();

	_lastTimer = currentTimer;

	return true;
}

