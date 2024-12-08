#pragma once
#include <chrono>
#include <fstream>

struct StatPacket
{
	__int64 id = 0;
	__int64 Start = 0;
	__int64 FrameCopy = 0;
	__int64 Encode = 0;
	__int64 QueuePop = 0;
	__int64 StreamSend = 0;
	__int64 RoundTrip = 0;
};


class Statistics
{
public:
	Statistics();
	~Statistics();
	void AddStat(StatPacket& packet);
	void PrintStat();
	void Reset();

private:
	__int64 FrameCopy = 0;
	__int64 Encode = 0;
	__int64 QueuePop = 0;
	__int64 StreamSend = 0;
	__int64 RoundTrip = 0;
	__int64 Total = 0;

	unsigned int count = 0;
	__int64 lastPacketID = 0;

#ifdef LOG_LATENCY
	std::ofstream file;
#endif
};

