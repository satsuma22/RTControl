#pragma once

#include "stream.hpp"

class DummyOpusStream : public StreamSource
{
    static const uint32_t defaultSamplesPerSecond = 50;

public:
    DummyOpusStream();
    virtual ~DummyOpusStream();

    virtual void start() override;
    virtual void stop() override;
    virtual void loadNextSample() override;

    rtc::binary getSample() override;
    uint64_t getSampleTime_us() override;
    uint64_t getSampleDuration_us() override;
    StatPacket getPacket() override { return StatPacket(); }

protected:
    rtc::binary sample = {};

private:
    uint64_t sampleDuration_us;
    uint64_t sampleTime_us = 0;
};

