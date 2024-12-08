#include "PacketQueue.h"

void PacketQueue::Insert(std::vector<uint8_t>& frame)
{
	m_bbMutex.lock();
	m_backBuffer = std::move(frame);
	m_bbMutex.unlock();

	m_stateMutex.lock();
	m_state++;
	m_stateMutex.unlock();

	swap();

	cv.notify_one();
}

void PacketQueue::Load(rtc::binary& sample, StatPacket& packet)
{
	{
		std::unique_lock<std::mutex> _lock(m_fbMutex);
		cv.wait(_lock, [this] {return this->IsReadReady(); });

		sample.resize(m_frontBuffer.size());
		memcpy(sample.data(), m_frontBuffer.data(), m_frontBuffer.size() * sizeof(std::byte));
		
		pFrontMutex.lock();
		packet = pFront;
		pFrontMutex.unlock();
	}
	
	m_stateMutex.lock();
	m_state--;
	m_stateMutex.unlock();

	swap();
}

void PacketQueue::swap()
{
	if (m_state != 1) return;

	// swap the buffers containing encoded video frames
	m_bbMutex.lock();
	m_fbMutex.lock();
	std::swap(m_frontBuffer, m_backBuffer);
	m_bbMutex.unlock();
	m_fbMutex.unlock();

	// swap the StatPackets
	pBackMutex.lock();
	pFrontMutex.lock();
	std::swap(pFront, pBack);
	pBackMutex.unlock();
	pFrontMutex.unlock();
}
