#include "rtc/rtc.hpp"

int main()
{
    rtc::Configuration config;
    config.iceServers.emplace_back("mystunserver.org:3478");

    rtc::PeerConnection pc(config);
}