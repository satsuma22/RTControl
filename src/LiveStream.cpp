#include "LiveStream.h"

#include <fstream>

#include "Streamer/h264Stream.h"
#include "Streamer/DummyOpusStream.h"

template <class T> std::weak_ptr<T> make_weak_ptr(std::shared_ptr<T> ptr) { return ptr; }

LiveStream::LiveStream(std::string title, HWND hwnd, DispatchQueue* mainThread)
{
    m_title = title;
    m_hwnd = hwnd;
    m_mainThread = mainThread;
    m_controller = WindowController(hwnd);

    ReadConfigFile();
}

LiveStream::~LiveStream()
{
    if (m_captureThread.joinable())
        m_captureThread.join();
}

void LiveStream::ReadConfigFile()
{
    std::fstream f("config.json");

    nlohmann::json settings = nlohmann::json::parse(f);

    m_settings.fps = settings["Streamer"]["fps"];
    m_settings.useMonitorCapture = settings["Global"]["CaptureMode"] == "Monitor";
    std::cout << "Streamer FPS: " << m_settings.fps << std::endl;
    std::cout << "Capture Mode: " << (m_settings.useMonitorCapture ? "Monitor" : "Window") << std::endl;

    f.close();
}

void LiveStream::CreatePeerConnection(const rtc::Configuration& config, std::weak_ptr<rtc::WebSocket> wws, std::string id)
{
    auto pc = std::make_shared<rtc::PeerConnection>(config);
    auto client = make_shared<Client>(pc);

    clients[id] = client;

    std::cout << "Setting up callbacks for peer Connection\n";

    pc->onStateChange([id, this](rtc::PeerConnection::State state) {
        std::cout << "State: " << state << std::endl;
        if (state == rtc::PeerConnection::State::Disconnected ||
            state == rtc::PeerConnection::State::Failed ||
            state == rtc::PeerConnection::State::Closed) {
            // remove disconnected client
            m_mainThread->dispatch([id, this]() {
                std::cout << "Removing Client: " << id << "from the list\n";
                clients.erase(id);
                });
        }
        });

    pc->onGatheringStateChange(
        [wpc = make_weak_ptr(pc), id, wws](rtc::PeerConnection::GatheringState state) {
            std::cout << "Gathering State: " << state << std::endl;
            if (state == rtc::PeerConnection::GatheringState::Complete) {
                std::cout << "Sending PeerConnection Offer\n";
                if (auto pc = wpc.lock()) {
                    auto description = pc->localDescription();
                    nlohmann::json message = {
                        {"id", id},
                        {"type", description->typeString()},
                        {"sdp", std::string(description.value())}
                    };
                    // Gathering complete, send answer
                    if (auto ws = wws.lock()) {
                        ws->send(message.dump());
                    }
                }
            }
        });

    client->video = AddVideo(pc, 102, 1, "video-stream", "stream1", [id, wc = make_weak_ptr(client), this]() {
        m_mainThread->dispatch([wc, this]() {
            if (auto c = wc.lock()) {
                AddToStream(c, true);
            }
            });
        std::cout << "Video from " << id << " opened" << std::endl;
        });

    client->audio = AddAudio(pc, 111, 2, "audio-stream", "stream1", [id, wc = make_weak_ptr(client), this]() {
        m_mainThread->dispatch([wc, this]() {
            if (auto c = wc.lock()) {
                AddToStream(c, false);
            }
            });
        std::cout << "Audio from " << id << " opened" << std::endl;
        });

    auto dc = pc->createDataChannel("ping-pong");
    dc->onOpen([id, wdc = make_weak_ptr(dc)]() {
        if (auto dc = wdc.lock()) {
            dc->send("Ping");
        }
        });

    dc->onMessage(nullptr, [id, wdc = make_weak_ptr(dc), this](std::string msg) {
        //std::cout << "Message from " << id << " received: " << msg << std::endl;
        if (auto dc = wdc.lock()) {
            nlohmann::json jsInput = nlohmann::json::parse(msg);
            auto it = jsInput.find("eventType");
            if (it != jsInput.end())
            {
                //std::cout << "Generating Input...\n";
                std::string eventType = it->get<std::string>();
                if (strcmp(eventType.c_str(), "KeyboardEvent") == 0)
                {
                    KeyboardEvent event;


                    event.altKey = jsInput["altKey"];
                    event.ctrlKey = jsInput["ctrlKey"];
                    event.shiftKey = jsInput["shiftKey"];
                    event.repeat = jsInput["repeat"];
                    event.code = jsInput["code"];
                    event.key = jsInput["key"];
                    event.type = (jsInput["type"] == "keydown") ? KeyboardEvent::TYPE::KEYDOWN : KeyboardEvent::TYPE::KEYUP;

                  
#ifdef PRINT_INPUT
                    std::cout << "Generating Keyboard Input...\n";
                    
                    if (event.altKey) std::cout << "AltKey: Yes\n";
                    else std::cout << "AltKey: No\n";
                    if (event.ctrlKey) std::cout << "crtlKey: Yes\n";
                    else std::cout << "ctrlKey: No\n";
                    if (event.shiftKey) std::cout << "shiftKey: Yes\n";
                    else std::cout << "shiftKey: No\n";
                    if (event.repeat) std::cout << "repeat: Yes\n";
                    else std::cout << "repeat: No\n";
                    std::cout << "Event Code: " << event.code << "\n";
                    std::cout << "Event Key: " << event.key << "\n";
                    if (event.type == KeyboardEvent::TYPE::KEYDOWN) std::cout << "KEYDOWN\n";
                    if (event.type == KeyboardEvent::TYPE::KEYUP) std::cout << "KEYUP\n";
                    std::cout << std::endl;
#endif
                    m_controller.GenerateInput(event, true);

                }
                else if (strcmp(eventType.c_str(), "MouseEvent") == 0)
                {

                    MouseEvent event;

                    event.altKey = jsInput["altKey"];
                    event.ctrlKey = jsInput["ctrlKey"];
                    event.shiftKey = jsInput["shiftKey"];
                    event.button = jsInput["button"];
                    event.offsetX = jsInput["offsetX"];
                    event.offsetY = jsInput["offsetY"];
                    std::string type = jsInput["type"];
                    if (type == "mousemove") event.type = MouseEvent::MOUSEMOVE;
                    else if (type == "mousedown") event.type = MouseEvent::MOUSEDOWN;
                    else if (type == "mouseup") event.type = MouseEvent::MOUSEUP;
                    
                    
#ifdef PRINT_INPUT
                    std::cout << "Generating Mouse Event\n";
                    
                    if (event.altKey) std::cout << "AltKey: Yes\n";
                    else std::cout << "AltKey: No\n";
                    if (event.ctrlKey) std::cout << "crtlKey: Yes\n";
                    else std::cout << "ctrlKey: No\n";
                    if (event.shiftKey) std::cout << "shiftKey: Yes\n";
                    else std::cout << "shiftKey: No\n";
                    std::cout << "Button: " << event.button << "\n";
                    std::cout << "offset: (" << event.offsetX << ", " << event.offsetY << ")\n";
                    if (event.type == MouseEvent::TYPE::MOUSEUP) std::cout << "MouseUp\n";
                    if (event.type == MouseEvent::TYPE::MOUSEDOWN) std::cout << "MouseDown\n";
                    if (event.type == MouseEvent::TYPE::MOUSEMOVE) std::cout << "MouseMove\n";
                    std::cout << std::endl;
#endif
                    m_controller.GenerateInput(event, m_settings.useMonitorCapture);
                }
                else if (strcmp(eventType.c_str(), "WheelEvent") == 0)
                {

                    WheelEvent event;

                    event.altKey = jsInput["altKey"];
                    event.ctrlKey = jsInput["ctrlKey"];
                    event.shiftKey = jsInput["shiftKey"];
                    event.button = jsInput["button"];
                    event.offsetX = jsInput["offsetX"];
                    event.offsetY = jsInput["offsetY"];
                    event.deltaX = jsInput["deltaX"];
                    event.deltaY = jsInput["deltaY"];


#ifdef PRINT_INPUT
                    std::cout << "Generating Mouse Wheel Event\n";
#endif
                    m_controller.GenerateInput(event);
                }
                else if (strcmp(eventType.c_str(), "TimePoint") == 0)
                {
                    //std::cout << "TimeStamp Received\n";
                    StatPacket packet;

                    packet.RoundTrip = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

                    packet.id = jsInput["ID"];
                    packet.Start = jsInput["Start"];
                    packet.FrameCopy = jsInput["FrameCopy"];
                    packet.Encode = jsInput["Encode"];
                    packet.QueuePop = jsInput["QueuePop"];
                    packet.StreamSend = jsInput["StreamSend"];

                    m_statistics.AddStat(packet);

                }

            }

        }
        });
    client->dataChannel = dc;

    pc->setLocalDescription();

}

void LiveStream::SetRemoteDescriptionForClient(nlohmann::json& message, std::string& id, std::string& type)
{

    if (auto jt = clients.find(id); jt != clients.end()) {
        std::cout << "Setting Remote Description for client...\n";
        auto pc = jt->second->peerConnection;
        auto sdp = message["sdp"].get<std::string>();
        auto description = rtc::Description(sdp, type);
        pc->setRemoteDescription(description);
    }
}

void LiveStream::StartCapture()
{
    auto isCaptureSupported = winrt::Windows::Graphics::Capture::GraphicsCaptureSession::IsSupported();
    if (!isCaptureSupported)
    {
        MessageBoxW(nullptr,
            L"Screen capture is not supported on this device for this release of Windows!",
            L"Win32CaptureSample",
            MB_OK | MB_ICONERROR);
        return;
    }

    m_captureApp = std::make_shared<CaptureApp>(&m_packetQueue, m_hwnd);
    m_captureThread = std::thread{ &LiveStream::LaunchCapturer, this, m_hwnd };

}

void LiveStream::LaunchCapturer(HWND hwnd)
{
    //LoadLibrary(L"C:/Windows/System32/nvEncodeAPI64.dll");
    LoadLibrary(L"nvEncodeAPI64.dll");

    winrt::init_apartment(winrt::apartment_type::single_threaded);

    auto controller = util::CreateDispatcherQueueControllerForCurrentThread();

    if (m_settings.useMonitorCapture)
    {
        HMONITOR hmon;
        DWORD flags = 0;
        hmon = MonitorFromWindow(hwnd, flags);

        m_captureApp->TryStartCaptureFromMonitorHandle(hmon);
    }
    else
    {
        m_captureApp->TryStartCaptureFromWindowHandle(hwnd);
    }


    // Message pump
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    util::ShutdownDispatcherQueueControllerAndWait(controller, static_cast<int>(msg.wParam));

}

std::shared_ptr<ClientTrackData> LiveStream::AddVideo(const std::shared_ptr<rtc::PeerConnection> pc, const uint8_t payloadType, const uint32_t ssrc, const std::string cname, const std::string msid, const std::function<void(void)> onOpen)
{
    auto video = rtc::Description::Video(cname);
    video.addH264Codec(payloadType);
    video.addSSRC(ssrc, cname, msid, cname);
    auto track = pc->addTrack(video);
    // create RTP configuration
    auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(ssrc, cname, payloadType, rtc::H264RtpPacketizer::defaultClockRate);
    // create packetizer
    //auto packetizer = std::make_shared<rtc::H264RtpPacketizer>(rtc::NalUnit::Separator::Length, rtpConfig);
    auto packetizer = std::make_shared<rtc::H264RtpPacketizer>(rtc::NalUnit::Separator::LongStartSequence, rtpConfig);
    // add RTCP SR handler
    auto srReporter = std::make_shared<rtc::RtcpSrReporter>(rtpConfig);
    packetizer->addToChain(srReporter);
    // add RTCP NACK handler
    auto nackResponder = std::make_shared<rtc::RtcpNackResponder>();
    packetizer->addToChain(nackResponder);
    // set handler
    track->setMediaHandler(packetizer);
    track->onOpen(onOpen);
    auto trackData = std::make_shared<ClientTrackData>(track, srReporter);
    return trackData;
}

std::shared_ptr<ClientTrackData> LiveStream::AddAudio(const std::shared_ptr<rtc::PeerConnection> pc, const uint8_t payloadType, const uint32_t ssrc, const std::string cname, const std::string msid, const std::function<void(void)> onOpen)
{
    auto audio = rtc::Description::Audio(cname);
    audio.addOpusCodec(payloadType);
    audio.addSSRC(ssrc, cname, msid, cname);
    auto track = pc->addTrack(audio);
    // create RTP configuration
    auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(ssrc, cname, payloadType, rtc::OpusRtpPacketizer::DefaultClockRate);
    // create packetizer
    auto packetizer = std::make_shared<rtc::OpusRtpPacketizer>(rtpConfig);
    // add RTCP SR handler
    auto srReporter = std::make_shared<rtc::RtcpSrReporter>(rtpConfig);
    packetizer->addToChain(srReporter);
    // add RTCP NACK handler
    auto nackResponder = std::make_shared<rtc::RtcpNackResponder>();
    packetizer->addToChain(nackResponder);
    // set handler
    track->setMediaHandler(packetizer);
    track->onOpen(onOpen);
    auto trackData = std::make_shared<ClientTrackData>(track, srReporter);
    return trackData;
}


/// Send previous key frame so browser can show something to user
/// @param stream Stream
/// @param video Video track data
void LiveStream::SendInitialNalus(std::shared_ptr<Stream> stream, std::shared_ptr<ClientTrackData> video)
{
    //auto h264 = dynamic_cast<H264FileParser*>(stream->video.get());
    auto h264 = dynamic_cast<h264Stream*>(stream->video.get());
    auto initialNalus = h264->initialNALUS();

    // send previous NALU key frame so users don't have to wait to see stream works
    if (!initialNalus.empty()) {
        const double frameDuration_s = double(h264->getSampleDuration_us()) / (1000 * 1000);
        const uint32_t frameTimestampDuration = video->sender->rtpConfig->secondsToTimestamp(frameDuration_s);
        video->sender->rtpConfig->timestamp = video->sender->rtpConfig->startTimestamp - frameTimestampDuration * 2;
        video->track->send(initialNalus);
        video->sender->rtpConfig->timestamp += frameTimestampDuration;
        // Send initial NAL units again to start stream in firefox browser
        video->track->send(initialNalus);
    }
}

std::shared_ptr<Stream> LiveStream::CreateStream(const unsigned fps)
{
    // video source
    //auto video = std::make_shared<H264FileParser>(h264Samples, fps, true, &m_renderQueue);
    auto video = std::make_shared<h264Stream>(fps, &m_packetQueue);
    // audio source
    //auto audio = std::make_shared<OPUSFileParser>(opusSamples, true);
    auto audio = std::make_shared<DummyOpusStream>();

    auto stream = std::make_shared<Stream>(video, audio);
    // set callback responsible for sample sending
    stream->onSample([ws = make_weak_ptr(stream), this](Stream::StreamSourceType type, uint64_t sampleTime, rtc::binary sample, StatPacket packet) {
        std::vector<std::shared_ptr<rtc::DataChannel>> dcs{};
        std::vector<ClientTrack> tracks{};
        std::string streamType = type == Stream::StreamSourceType::Video ? "video" : "audio";
        // get track for given type
        std::function<std::optional<std::shared_ptr<ClientTrackData>>(std::shared_ptr<Client>)> getTrackData = [type](std::shared_ptr<Client> client) {
            return type == Stream::StreamSourceType::Video ? client->video : client->audio;
            };

        // get all clients with Ready state
        for (auto id_client : clients) {
            auto id = id_client.first;
            auto client = id_client.second;
            auto optTrackData = getTrackData(client);
            if (client->getState() == Client::State::Ready && optTrackData.has_value()) {
                auto trackData = optTrackData.value();
                tracks.push_back(ClientTrack(id, trackData));
                dcs.push_back(client->dataChannel.value());
            }
        }
        if (!tracks.empty()) {
            for (auto clientTrack : tracks) {
                auto client = clientTrack.id;
                auto trackData = clientTrack.trackData;
                auto rtpConfig = trackData->sender->rtpConfig;

                // sample time is in us, we need to convert it to seconds
                auto elapsedSeconds = double(sampleTime) / (1000 * 1000);
                // get elapsed time in clock rate
                uint32_t elapsedTimestamp = rtpConfig->secondsToTimestamp(elapsedSeconds);
                // set new timestamp
                rtpConfig->timestamp = rtpConfig->startTimestamp + elapsedTimestamp;

                // get elapsed time in clock rate from last RTCP sender report
                auto reportElapsedTimestamp = rtpConfig->timestamp - trackData->sender->lastReportedTimestamp();
                // check if last report was at least 1 second ago
                if (rtpConfig->timestampToSeconds(reportElapsedTimestamp) > 1) {
                    trackData->sender->setNeedsToReport();
                }

                //std::cout << "Sending " << streamType << " sample with size: " << std::to_string(sample.size()) << " to " << client << std::endl;
                try {
                    // send sample
                    trackData->track->send(sample);
                }
                catch (const std::exception& e) {
                    std::cerr << "Unable to send " << streamType << " packet: " << e.what() << std::endl;
                }
            }

            // Send the StatPacket to the client to calculate the Round Trip Time
            if (type == Stream::StreamSourceType::Video)
            {
                packet.StreamSend = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                nlohmann::json tmp = {
                    {"eventType", "TimePoint"},
                    { "ID", packet.id},
                    { "Start", packet.Start },
                    { "FrameCopy", packet.FrameCopy } ,
                    { "Encode", packet.Encode } ,
                    { "QueuePop", packet.QueuePop },
                    { "StreamSend", packet.StreamSend }
                };



                for (auto& dc : dcs)
                {

                    try {
                        dc->send(tmp.dump());
                    }
                    catch (const std::exception& e) {
                        std::cerr << "Unable to send timestamp packet: " << e.what() << std::endl;
                    }
                }
            }
        }
        m_mainThread->dispatch([ws, this]() {
            if (clients.empty()) {
                // we have no clients, stop the stream
                if (auto stream = ws.lock()) {
                    stream->stop();
                }
            }
            });
        });
    return stream;
}

void LiveStream::StartStream()
{
    std::shared_ptr<Stream> stream;
    if (avStream.has_value()) {
        stream = avStream.value();
        if (stream->isRunning) {
            // stream is already running
            return;
        }
    }
    else {
        stream = CreateStream(m_settings.fps);
        avStream = stream;
    }
    stream->start();
}

void LiveStream::AddToStream(std::shared_ptr<Client> client, bool isAddingVideo)
{
    if (client->getState() == Client::State::Waiting) {
        client->setState(isAddingVideo ? Client::State::WaitingForAudio : Client::State::WaitingForVideo);
    }
    else if ((client->getState() == Client::State::WaitingForAudio && !isAddingVideo)
        || (client->getState() == Client::State::WaitingForVideo && isAddingVideo)) {

        // Audio and video tracks are collected now
        assert(client->video.has_value() && client->audio.has_value());
        auto video = client->video.value();

        if (avStream.has_value()) {
            SendInitialNalus(avStream.value(), video);
        }

        client->setState(Client::State::Ready);
    }
    if (client->getState() == Client::State::Ready) {
        StartStream();
    }
}
