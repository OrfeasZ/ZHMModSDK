#pragma once

#include <expected>
#include <string>
#include <cstdint>

#include "EntityTreeNode.h"

#include "uwebsockets/App.h"
#include <Glacier/ZMath.h>
#include <Glacier/ZResourceID.h>
#include <Glacier/ZEntity.h>

#include <simdjson.h>

struct EntitySelector {
	uint64_t EntityId;
	std::optional<ZRuntimeResourceID> TbluHash;
};

class EditorServer {
public:
	struct SocketUserData {
		std::string Identifier;
	};

	using WebSocket = uWS::WebSocket<false, true, SocketUserData>;

	EditorServer();
	~EditorServer();

	void OnEntitySelected(ZEntityRef p_Entity);
	void OnEntityTransformChanged(ZEntityRef p_Entity);
	void OnEntityNameChanged(ZEntityRef p_Entity);
	void OnEntityPropertySet(ZEntityRef p_Entity, uint32_t p_PropertyId);
	void OnSceneLoading(const std::string& p_Scene, const std::vector<std::string>& p_Bricks);
	void OnSceneClearing(bool p_ForReload);

private:
	static void OnMessage(WebSocket* p_Socket, std::string_view p_Message) noexcept(false);

	static void SendWelcome(WebSocket* p_Socket);
	static void SendError(WebSocket* p_Socket, std::string p_Message);
	static void SendEntityList(WebSocket* p_Socket, std::shared_ptr<EntityTreeNode> p_Tree);
	static void SendEntityDetails(WebSocket* p_Socket, ZEntityRef p_Entity);

	static void WriteEntityDetails(std::ostream& p_Stream, ZEntityRef p_Entity);
	static void WriteVector3(std::ostream& p_Stream, double p_X, double p_Y, double p_Z);
	static void WriteRotation(std::ostream& p_Stream, double p_Yaw, double p_Pitch, double p_Roll);
	static void WriteTransform(std::ostream& p_Stream, SMatrix p_Transform);
	static void WritePropertyName(std::ostream& p_Stream, ZEntityProperty* p_Property);
	static void WriteProperty(std::ostream& p_Stream, ZEntityRef p_Entity, ZEntityProperty* p_Property);

	static EntitySelector ReadEntitySelector(simdjson::ondemand::value p_Selector);
	static SVector3 ReadVector3(simdjson::ondemand::value p_Vector);
	static EulerAngles ReadRotation(simdjson::ondemand::value p_Rotation);
	static SMatrix ReadTransform(simdjson::ondemand::value p_Transform);
	static ZRuntimeResourceID ReadResourceId(simdjson::ondemand::value p_ResourceId);
	static uint64_t ReadEntityId(simdjson::ondemand::value p_EntityId);

private:
	uWS::App* m_App;
	uWS::Loop* m_Loop;
	std::jthread m_ServerThread;
};