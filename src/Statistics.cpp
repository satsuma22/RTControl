#include "Statistics.h"
#include <algorithm>
#include <iostream>

Statistics::Statistics()
{
#ifdef LOG_LATENCY
	file.open("latencies.csv");

	if (!file.is_open())
	{
		std::cout << "Error opening file for stats logging\n";
	}

	file << "Frame#\t\t,FrameCopy\t\t,Encode\t\t,\t\tQueuePop\t\t,StreamSend\t\t,RTT\t\t,Total\t\t," << std::endl;
#endif
}

Statistics::~Statistics()
{
#ifdef LOG_LATENCY
	if (file.is_open())
		file.close();
#endif
}

void Statistics::AddStat(StatPacket& packet)
{
	//if (0)
	if (packet.id < lastPacketID)
	{
		std::cout << "PacketID Out of Order\n";
		return;
	}
	lastPacketID = packet.id;


	FrameCopy += packet.FrameCopy - packet.Start;
	Encode += packet.Encode - packet.FrameCopy;
	QueuePop += packet.QueuePop - packet.Encode;
	StreamSend += packet.StreamSend - packet.QueuePop;
	RoundTrip += packet.RoundTrip - packet.StreamSend;
	Total += packet.RoundTrip - packet.Start;
	
#ifdef LOG_LATENCY
	file << packet.id << "\t\t," << packet.FrameCopy - packet.Start << "\t\t," << packet.Encode - packet.FrameCopy << "\t\t," << packet.QueuePop - packet.Encode << "\t\t," << packet.StreamSend - packet.QueuePop << "\t\t," << packet.RoundTrip - packet.StreamSend << "\t\t," << packet.RoundTrip - packet.Start << "\t\t," << std::endl;
#endif

	count++;

	if (count % 600 == 0)
		PrintStat();
}

void Statistics::PrintStat()
{
	std::cout << "FrameCopy:    " << (double)FrameCopy / count << "\n";
	std::cout << "Encode:       " << (double)Encode / count << "\n";
	std::cout << "QueuePop:     " << (double)QueuePop / count << "\n";
	std::cout << "StreamSend:   " << (double)StreamSend / count << "\n";
	std::cout << "RoundTrip:    " << (double)RoundTrip / count << "\n";
	std::cout << "Total:	" << (double)Total / count << "\n";

	Reset();
}

void Statistics::Reset()
{
	count = 0;

	FrameCopy = 0;
	Encode = 0;
	QueuePop = 0;
	StreamSend = 0;
	RoundTrip = 0;
	Total = 0;
}
