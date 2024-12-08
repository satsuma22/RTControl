#include "App.h"
#include <fstream>

void App::Init()
{

	ReadConfigFile();

	std::string stunServer = "stun:stun.l.google.com:19302";
	std::cout << "STUN server is " << stunServer << std::endl;

	m_config.iceServers.emplace_back(stunServer);
	m_config.disableAutoNegotiation = true;

	m_webSocket = std::make_shared<rtc::WebSocket>();
	m_webSocket->onOpen([]() { std::cout << "WebSocket connected, signaling ready" << std::endl; });

	m_webSocket->onClosed([]() { std::cout << "WebSocket closed" << std::endl; });

	m_webSocket->onError([](const std::string& error) { std::cout << "WebSocket failed: " << error << std::endl; });

	m_webSocket->onMessage([&](std::variant<rtc::binary, std::string> data) {
		if (!holds_alternative<std::string>(data))
			return;

		rtc::Configuration& config = m_config;
		std::shared_ptr<rtc::WebSocket> ws = m_webSocket;

		nlohmann::json message = nlohmann::json::parse(get<std::string>(data));
		MainThread.dispatch([message, config, ws, this]() {
			wsOnMessage(message, config, ws);
			});
		});


	m_webSocket->open("ws://127.0.0.1:8000/server");

	std::cout << "Waiting for signaling to be connected..." << std::endl;
	while (!m_webSocket->isOpen()) {
		if (m_webSocket->isClosed())
			return;
		//Sleep(1000);
	}
}

void App::Run()
{
	bool run = true;
	while (run) {
		std::string id;
		std::cout << "Enter to exit" << std::endl;
		std::cin >> id;
		std::cin.ignore();
		std::cout << "exiting" << std::endl;
		break;
	}
}

void App::Close()
{
	std::cout << "Cleaning up..." << std::endl;
	m_webSocket->close();
}


void App::InitWithStartCapture(std::string title)
{
    m_title = title;
    Init();
    HWND hwnd;
    hwnd = FindWindowA(nullptr, title.c_str());
    m_liveStream = std::make_unique<LiveStream>(title, hwnd, &MainThread);
    m_liveStream->StartCapture();
}

bool App::LaunchInstance(std::string& title)
{
    std::string cmd = m_appSettings.applications[title];
    m_title = title;

    // Launch the application and detach
    std::thread t(system, cmd.c_str());
    t.detach();


    HWND hwnd = nullptr;
    while (hwnd == nullptr)
    {
        //Sleep(1000);
        hwnd = FindWindowA(nullptr, title.c_str());
    }
    std::cout << "HWND: " << hwnd << std::endl;
    m_liveStream = std::make_unique<LiveStream>(title, hwnd, &MainThread);
    m_liveStream->StartCapture();

    return true;
}

void App::wsOnMessage(nlohmann::json message, rtc::Configuration config, std::shared_ptr<rtc::WebSocket> ws)
{

    auto it = message.find("id");
    if (it == message.end())
        return;
    std::string id = it->get<std::string>();

    it = message.find("type");
    if (it == message.end())
        return;

    std::string type = it->get<std::string>();
    std::cout << "Client: " << id << " wants to " << type << std::endl;

    if (type == "requestPeerConnection")
    {
        std::cout << "Creating Peer Connection\n";
        m_liveStream->CreatePeerConnection(m_config, make_weak_ptr(m_webSocket), id);

    }
    else if (type == "requestStatus")
    {
        SendStatusToClient(id);
    }
    else if (type == "requestStartStream")
    {
        auto titleIt = message.find("app");
        if (titleIt == message.end())
        {
            std::cout << "Couldn't find App\n";
            return;
        }
        std::string appTitle = titleIt->get<std::string>();
        std::cout << "Launching application: " << appTitle;
        LaunchInstance(appTitle);

        AddClientToStream(id);

    }
    else if (type == "requestJoinStream")
    {
        if (!m_liveStream)
            return;
        AddClientToStream(id);
    }
    else if (type == "answer") {
        m_liveStream->SetRemoteDescriptionForClient(message, id, type);
    }

}

void App::AddClientToStream(std::string& id)
{
    std::cout << "Creating Peer Connection\n";
    m_liveStream->CreatePeerConnection(m_config, make_weak_ptr(m_webSocket), id);
}

void App::SendStatusToClient(std::string& id)
{
    // Send a list of all available and running application instances
    nlohmann::json message = {
        {"id", id},
        {"type", "Status"}
    };

    if (m_liveStream)
    {
        nlohmann::json appList = nlohmann::json::array();
        appList.push_back(m_title);

        message["isRunning"] = 1;
        message["list"] = m_title;
     
    }
    else
    {

        nlohmann::json appList = nlohmann::json::array();

        for (auto& pair : m_appSettings.applications)
            appList.push_back(pair.first);

        message["isRunning"] = 0;
        message["list"] = appList;
    }

    auto wws = make_weak_ptr(m_webSocket);

    if (auto ws = wws.lock()) {
        std::cout << "Sending status...\n" << message.dump() << "\n";
        ws->send(message.dump());
    }

}

void App::WriteDefaultConfigFile()
{
    std::cout << "Writing Default Config File\n";
    nlohmann::json settings;

    //Application Settings
    settings["Global"]["List"]["Untitled - Notepad"] = "notepad.exe";
    settings["Global"]["List"]["Untitled - Paint"] = "mspaint.exe";
    settings["Global"]["CaptureMode"] = "Monitor";
    settings["Global"]["CaptureMode_comment"] = "Monitor / Window";
    settings["Global"]["CaptureCursor"] = 1;

    //Streamer Settings
    settings["Streamer"]["fps"] = 120;

    //Encoder Settings
    settings["Encoder"]["Bitrate Average"] = 40000000;
    settings["Encoder"]["Bitrate Max"] = 40000000;
    settings["Encoder"]["fps"] = 60;
    settings["Encoder"]["GOP Length"] = 60;
    settings["Encoder"]["IDR Period"] = 60;
    settings["Encoder"]["I-Frames Only"] = 0;
    settings["Encoder"]["OutputDelay"] = 1;
    settings["Encoder"]["Preset"] = 4;
    settings["Encoder"]["Rate Control Mode"] = "CBR";
    settings["Encoder"]["Rate Control Mode Comment"] = "CBR / VBR / CQP / Target Quality";
    settings["Encoder"]["QP"] = 25;
    settings["Encoder"]["Target Quality"] = 25;


    std::ofstream o("config.json");

    o << std::setw(8) << settings << std::endl;
    o.close();

}

void App::ReadConfigFile()
{
    std::fstream f("config.json");
    if (!f.good())
    {
        WriteDefaultConfigFile();
        f = std::fstream("config.json");
    }
    nlohmann::json data = nlohmann::json::parse(f);

    auto map = data["Global"]["List"];

    for (auto x : map.items())
        m_appSettings.applications[x.key()] = x.value();

    std::cout << "List of Applications:\n";
    for (auto entry : m_appSettings.applications)
        std::cout << entry.first << ", " << entry.second << "\n";

    f.close();
}

