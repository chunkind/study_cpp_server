#include "pch.h"
#include "GameRoom.h"
#include "Player.h"
#include "Monster.h"
#include "GameSession.h"

GameRoomRef GRoom = make_shared<GameRoom>();

GameRoom::GameRoom()
{
}

GameRoom::~GameRoom()
{

}

void GameRoom::Init()
{
	MonsterRef monster = GameObject::CreateMonster();
	monster->info.set_posx(8);
	monster->info.set_posy(8);
	AddObject(monster);
}

void GameRoom::Update()
{

}

void GameRoom::EnterRoom(GameSessionRef session)
{
	PlayerRef player = GameObject::CreatePlayer();

	// ������ ���縦 ����
	session->gameRoom = GetRoomRef();
	session->player = player;
	player->session = session;

	//Temp
	player->info.set_posx(5);
	player->info.set_posy(5);

	// ������ Ŭ�󿡰� ������ �����ֱ�
	{
		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_MyPlayer(player->info);
		session->Send(sendBuffer);
	}
	// ��� ������Ʈ ���� ����
	{
		Protocol::S_AddObject pkt;

		for (auto& item : _players)
		{
			Protocol::ObjectInfo* info = pkt.add_objects();
			*info = item.second->info;
		}

		for (auto& item : _monsters)
		{
			Protocol::ObjectInfo* info = pkt.add_objects();
			*info = item.second->info;
		}

		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_AddObject(pkt);
		session->Send(sendBuffer);
	}

	AddObject(player);
}

void GameRoom::LeaveRoom(GameSessionRef session)
{
	if (session == nullptr)
		return;
	if (session->player.lock() == nullptr)
		return;

	uint64 id = session->player.lock()->info.objectid();
	RemoveObject(id);
}

GameObjectRef GameRoom::FindObject(uint64 id)
{
	{
		auto findIt = _players.find(id);
		if (findIt != _players.end())
			return findIt->second;
	}
	{
		auto findIt = _monsters.find(id);
		if (findIt != _monsters.end())
			return findIt->second;
	}

	return nullptr;
}

void GameRoom::Handle_C_Move(Protocol::C_Move& pkt)
{
	uint64 id = pkt.info().objectid();
	GameObjectRef gameObject = FindObject(id);
	if (gameObject == nullptr)
		return;

	// TODO : Validation
	gameObject->info.set_state(pkt.info().state());
	gameObject->info.set_dir(pkt.info().dir());
	gameObject->info.set_posx(pkt.info().posx());
	gameObject->info.set_posy(pkt.info().posy());

	{
		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Move(pkt.info());
		Broadcast(sendBuffer);
	}
}

void GameRoom::AddObject(GameObjectRef gameObject)
{
	uint64 id = gameObject->info.objectid();

	auto objectType = gameObject->info.objecttype();

	switch (objectType)
	{
	case Protocol::OBJECT_TYPE_PLAYER:
		_players[id] = static_pointer_cast<Player>(gameObject);
		break;
	case Protocol::OBJECT_TYPE_MONSTER:
		_monsters[id] = static_pointer_cast<Monster>(gameObject);
		break;
	default:
		return;
	}

	gameObject->room = GetRoomRef();

	// �ű� ������Ʈ ���� ����
	{
		Protocol::S_AddObject pkt;

		Protocol::ObjectInfo* info = pkt.add_objects();
		*info = gameObject->info;

		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_AddObject(pkt);
		Broadcast(sendBuffer);
	}
}

void GameRoom::RemoveObject(uint64 id)
{
	GameObjectRef gameObject = FindObject(id);
	if (gameObject == nullptr)
		return;

	switch (gameObject->info.objecttype())
	{
	case Protocol::OBJECT_TYPE_PLAYER:
		_players.erase(id);
		break;
	case Protocol::OBJECT_TYPE_MONSTER:
		_monsters.erase(id);
		break;
	default:
		return;
	}

	gameObject->room = nullptr;

	// ������Ʈ ���� ����
	{
		Protocol::S_RemoveObject pkt;
		pkt.add_ids(id);

		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_RemoveObject(pkt);
		Broadcast(sendBuffer);
	}
}

void GameRoom::Broadcast(SendBufferRef& sendBuffer)
{
	for (auto& item : _players)
	{
		item.second->session->Send(sendBuffer);
	}
}