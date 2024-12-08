#pragma once

#include <rtc/rtc.hpp>
#include <vector>
#include <mutex>
#include <iostream>
#include <condition_variable>
#include "Statistics.h"

class PacketQueue
{
public:
	void Insert(std::vector<uint8_t>& frame);
	void Load(rtc::binary& sample, StatPacket& packet);

	bool IsReadReady() const { return m_state > 0; }
	bool IsWriteReady() const { return m_state < 2; }

	void SetBackPacket(StatPacket& packet) { pBackMutex.lock(); pBack = packet; pBackMutex.unlock(); }

private:
	void swap();

private:
	// 0: front buffer : Not Updated, back buffer : Not Updated -> back buffer is ready to be written
	// 1: front buffer : Updated    , back buffer : Not Updated -> front buffer is ready to be read AND back buffer is ready to be written
	// 2: front buffer : Updated	, back buffer : Updated		-> front buffer is ready to be read
	unsigned int m_state = 0;
	std::vector<uint8_t> m_backBuffer;
	std::vector<uint8_t> m_frontBuffer;
	std::mutex m_bbMutex;
	std::mutex m_fbMutex;
	std::mutex m_stateMutex;

	std::condition_variable cv;

public:

	StatPacket pFront;
	StatPacket pBack;

	std::mutex pFrontMutex;
	std::mutex pBackMutex;
};


