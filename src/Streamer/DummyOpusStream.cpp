#include "DummyOpusStream.h"

DummyOpusStream::DummyOpusStream()
{
	sampleDuration_us = 1000 * 1000 / defaultSamplesPerSecond;
}

DummyOpusStream::~DummyOpusStream()
{
	stop();
}

void DummyOpusStream::start()
{
	sampleTime_us = std::numeric_limits<uint64_t>::max() - sampleDuration_us + 1;
	loadNextSample();
}

void DummyOpusStream::stop()
{
	sample = {};
	sampleTime_us = 0;
}

void DummyOpusStream::loadNextSample()
{
	sampleTime_us += sampleDuration_us;
}

rtc::binary DummyOpusStream::getSample()
{
	return sample;
}

uint64_t DummyOpusStream::getSampleTime_us()
{
	return sampleTime_us;
}

uint64_t DummyOpusStream::getSampleDuration_us()
{
	return sampleDuration_us;
}
