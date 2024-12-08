#pragma once

#include <Windows.h>

#include <memory>
#include <unordered_map>

#include <rtc/rtc.hpp>
#include <rtc/websocket.hpp>
#include <rtc/configuration.hpp>
#include <nlohmann/json.hpp>

#include "Streamer/dispatchqueue.hpp"
#include "LiveStream.h"

template <class T> std::weak_ptr<T> make_weak_ptr(std::shared_ptr<T> ptr) { return ptr; }

struct AppSettings
{
	std::unordered_map<std::string, std::string> applications;
};

class App
{
public:
	App() {};
	~App() {};

	void Init();
	// Initializes the App and starts capturing the window with the provided title
	void InitWithStartCapture(std::string title);
	void Run();
	void Close();

private:
	// Sets 'onMessage' callbacks for the websocket
	void wsOnMessage(nlohmann::json message, rtc::Configuration config, std::shared_ptr<rtc::WebSocket> ws);
	bool LaunchInstance(std::string& titile);
	// Adds a client to a stream
	void AddClientToStream(std::string& id);
	void SendStatusToClient(std::string& id);

	void WriteDefaultConfigFile();
	void ReadConfigFile();

private:
	DispatchQueue MainThread = DispatchQueue("Main", 4);
	rtc::Configuration m_config;
	std::shared_ptr<rtc::WebSocket> m_webSocket;
	std::unique_ptr<LiveStream> m_liveStream;
	AppSettings m_appSettings;
	std::string m_title = "";
};

