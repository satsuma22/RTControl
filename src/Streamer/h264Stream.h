#pragma once

#include "stream.hpp"
#include "../PacketQueue.h"

#include <optional>
#include <vector>

class h264Stream : public StreamSource
{
public:
    h264Stream(uint32_t fps, PacketQueue* packetQueue);
    virtual ~h264Stream();

    virtual void start() override;
    virtual void stop() override;
    virtual void loadNextSample() override;

    rtc::binary getSample() override;
    uint64_t getSampleTime_us() override;
    uint64_t getSampleDuration_us() override;
    StatPacket getPacket() override { return statPacket; }

    std::vector<std::byte> initialNALUS();

protected:
	rtc::binary sample = {};

private:
	uint64_t sampleDuration_us;
	uint64_t sampleTime_us = 0;
    PacketQueue* m_packetQueue;
    StatPacket statPacket;

    std::optional<std::vector<std::byte>> previousUnitType5 = std::nullopt;
    std::optional<std::vector<std::byte>> previousUnitType7 = std::nullopt;
    std::optional<std::vector<std::byte>> previousUnitType8 = std::nullopt;
};

