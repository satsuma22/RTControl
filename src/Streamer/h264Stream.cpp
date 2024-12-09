#include "h264Stream.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif


h264Stream::h264Stream(uint32_t fps, PacketQueue* packetQueue)
{
	sampleDuration_us = 1000 * 1000 / fps;
	m_packetQueue = packetQueue;
}

h264Stream::~h264Stream()
{
	stop();
}

void h264Stream::start()
{
	sampleTime_us = std::numeric_limits<uint64_t>::max() - sampleDuration_us + 1;
	loadNextSample();
}

void h264Stream::stop()
{
	sample = {};
	sampleTime_us = 0;
}

void h264Stream::loadNextSample()
{
	sampleTime_us += sampleDuration_us;
	
    {
        m_packetQueue->Load(sample, statPacket);
        statPacket.QueuePop = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    size_t i = 0;
    while (i < sample.size())
    {
        assert(i + 4 < sample.size());
        auto naluStartIndex = i + 4;
        auto naluEndIndex = naluStartIndex;
        
        auto header = reinterpret_cast<rtc::NalUnitHeader*>(sample.data() + naluStartIndex);
        auto type = header->unitType();
#ifdef PRINT_DEBUG_INFO
        std::cout << "(NALU) Type: " << (int)type << "\n";
#endif
        //if (type == 1) break;

        while (naluEndIndex < sample.size())
        {
            i++;

            auto xptr = (uint32_t*)(sample.data() + i);
            uint32_t x = ntohl(*xptr);
            if (x == 1)
            {
                naluEndIndex = i;
                break;
            }
            naluEndIndex = i;
        }

        assert(naluEndIndex <= sample.size());
       
        switch (type) {
        case 7:
            previousUnitType7 = { sample.begin() + i, sample.begin() + naluEndIndex };
            break;
        case 8:
            previousUnitType8 = { sample.begin() + i, sample.begin() + naluEndIndex };
            break;
        case 5:
            previousUnitType5 = { sample.begin() + i, sample.begin() + naluEndIndex };
            break;
        }
    }

}

rtc::binary h264Stream::getSample()
{
	return sample;
}

uint64_t h264Stream::getSampleTime_us()
{
	return sampleTime_us;
}

uint64_t h264Stream::getSampleDuration_us()
{
	return sampleDuration_us;
}

std::vector<std::byte> h264Stream::initialNALUS()
{
    std::vector<std::byte> units{};
    if (previousUnitType7.has_value()) {
        auto nalu = previousUnitType7.value();
        units.insert(units.end(), nalu.begin(), nalu.end());
    }
    if (previousUnitType8.has_value()) {
        auto nalu = previousUnitType8.value();
        units.insert(units.end(), nalu.begin(), nalu.end());
    }
    if (previousUnitType5.has_value()) {
        auto nalu = previousUnitType5.value();
        units.insert(units.end(), nalu.begin(), nalu.end());
    }
    return units;
}
