//new

#include "pch.h"
#include "ServerPacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"

void ServerPacketHandler::HandlePacket(BYTE* buffer, int32 len)
{
	BufferReader br(buffer, len);

	PacketHeader header;
	br.Peek(&header);

	switch (header.id)
	{
	default:
		break;
	}
}

// [size | 1][ id hp attack]
SendBufferRef ServerPacketHandler::Make_S_TEST(uint64 id, uint32 hp, uint16 attack, vector<BuffData> buffs)
{
	SendBufferRef sendBuffer = make_shared<SendBuffer>(4096);

	BufferWriter bw(sendBuffer->Buffer(), sendBuffer->Capacity());

	PacketHeader* header = bw.Reserve<PacketHeader>();
	// id(uint64), ü��(uint32), ���ݷ�(uint16)
	bw << id << hp << attack;

	// ���� ������
	bw << (uint16)buffs.size();
	for (BuffData& buff : buffs)
	{
		bw << buff.buffId << buff.remainTime;
	}

	header->size = bw.WriteSize();
	header->id = S_TEST; // 1 : Test Msg

	sendBuffer->Close(bw.WriteSize());

	return sendBuffer;
}