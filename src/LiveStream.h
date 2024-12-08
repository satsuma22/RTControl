#pragma once

#include <Windows.h>

#include "Streamer/helpers.hpp"
#include "Streamer/stream.hpp"
#include "Streamer/dispatchqueue.hpp"
#include "PacketQueue.h"
#include "Capturer/CaptureApp.h"
#include "Capturer/util.h"
#include "Controller.h"
#include "Statistics.h"

#include <nlohmann/json.hpp>

struct StreamerSettings
{
	unsigned int fps = 60;
	bool useMonitorCapture = true;
};

class LiveStream
{
public:
	LiveStream(std::string title, HWND hwnd, DispatchQueue* mainThread);
	~LiveStream();

	void StartCapture();


	// Creates sdp offer for the client
	void CreatePeerConnection(const rtc::Configuration& config,
		std::weak_ptr<rtc::WebSocket> wws,
		std::string id);

	// Sets the sdp answer received from the client
	void SetRemoteDescriptionForClient(nlohmann::json& message, std::string& id, std::string& type);

private:
	std::shared_ptr<ClientTrackData> AddVideo(const std::shared_ptr<rtc::PeerConnection> pc, const uint8_t payloadType, const uint32_t ssrc, const std::string cname, const std::string msid, const std::function<void(void)> onOpen);
	std::shared_ptr<ClientTrackData> AddAudio(const std::shared_ptr<rtc::PeerConnection> pc, const uint8_t payloadType, const uint32_t ssrc, const std::string cname, const std::string msid, const std::function<void(void)> onOpen);
	void SendInitialNalus(std::shared_ptr<Stream> stream, std::shared_ptr<ClientTrackData> video);
	std::shared_ptr<Stream> CreateStream(const unsigned fps);
	void StartStream();
	void AddToStream(std::shared_ptr<Client> client, bool isAddingVideo);

	void ReadConfigFile();
	void LaunchCapturer(HWND hwnd);

private:
	std::string m_title;
	HWND m_hwnd;

	PacketQueue m_packetQueue;
	std::shared_ptr<CaptureApp> m_captureApp;
	WindowController m_controller;
	Statistics m_statistics;

	DispatchQueue* m_mainThread;
	std::thread m_captureThread;

	/// all connected clients
	std::unordered_map<std::string, std::shared_ptr<Client>> clients{};
	/// Audio and video stream
	std::optional<std::shared_ptr<Stream>> avStream = std::nullopt;

	StreamerSettings m_settings;
};

