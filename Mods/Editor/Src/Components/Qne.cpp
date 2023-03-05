#include <Editor.h>

#include "Qne.h"

#include "Logging.h"

void Editor::CheckQneConnection(float p_DeltaTime)
{
    m_QneConnectionTimer += p_DeltaTime;

    // Check QNE connection every 10 seconds.
    if (m_QneConnectionTimer < 10.f)
        return;

    m_QneConnectionTimer = 0.f;

    SendQneMessage(Qne::SdkToQnMessage_Hello { .protocol_version = Qne::PROTOCOL_VERSION });
}

void Editor::ReceiveQneMessages()
{
    Qne::MessageContainer s_MessageContainer {};

    while (true)
    {
        const int s_ReadBytes = recvfrom(m_QneSocket, reinterpret_cast<char*>(&s_MessageContainer), sizeof(s_MessageContainer), 0, nullptr, nullptr);

        if (s_ReadBytes == SOCKET_ERROR)
        {
            // No more messages to read.
            if (WSAGetLastError() == WSAEWOULDBLOCK)
                break;

            Logger::Error("Failed to read QNE message (err = {}, len = {})", WSAGetLastError(), s_ReadBytes);
            m_ConnectedToQne = false;
            return;
        }

        if (s_ReadBytes <= sizeof(uint32_t))
        {
            Logger::Error("Failed to read QNE message (err = {}, len = {})", WSAGetLastError(), s_ReadBytes);
            m_ConnectedToQne = false;
            return;
        }

        Logger::Debug("Read {} bytes from QNE", s_ReadBytes);

        try
        {
            auto s_Message = Qne::ParseQnToSdkMessage(s_MessageContainer);

            std::visit(Qne::Overload {
                [this](const Qne::QnToSdkMessage_Hello& p_Msg)
                {
                    if (p_Msg.protocol_version != Qne::PROTOCOL_VERSION)
                    {
                        Logger::Error("QNE protocol version mismatch (expected {}, got {})", Qne::PROTOCOL_VERSION, p_Msg.protocol_version);

                        if (m_ConnectedToQne)
                            Logger::Warn("Disconnected from QNE.");

                        m_ConnectedToQne = false;
                    }
                    else
                    {
                        if (!m_ConnectedToQne)
                            Logger::Info("Connected to QNE (protocol version = {})!", p_Msg.protocol_version);
                        else
                            Logger::Debug("QNE still alive!");

                        m_ConnectedToQne = true;
                    }
                },
                [this](const Qne::QnToSdkMessage_SelectEntity& p_Msg)
                {
                    Logger::Debug("QNE select entity {:x} in tblu {}", p_Msg.entity_id, p_Msg.tblu_hash);
                    // TODO
                },
                [this](const Qne::QnToSdkMessage_SetEntityTransform& p_Msg)
                {
                    Logger::Debug("QNE set entity transform {:x} in tblu {}", p_Msg.entity_id, p_Msg.tblu_hash);
                    Logger::Debug("rotation = ({}, {}, {})", p_Msg.transform.rotation.x, p_Msg.transform.rotation.y, p_Msg.transform.rotation.z);
                    Logger::Debug("position = ({}, {}, {})", p_Msg.transform.position.x, p_Msg.transform.position.y, p_Msg.transform.position.z);
                    // TODO
                },
                [this](const Qne::QnToSdkMessage_SpawnEntity& p_Msg)
                {
                    Logger::Debug("QNE spawn entity {:x} with temp {}", p_Msg.entity_id, p_Msg.temp_hash);
                    // TODO
                },
                [this](const Qne::QnToSdkMessage_SetSpawnedEntityTransform& p_Msg)
                {
                    Logger::Debug("QNE set spawned entity transform {:x}", p_Msg.entity_id);
                    Logger::Debug("rotation = ({}, {}, {})", p_Msg.transform.rotation.x, p_Msg.transform.rotation.y, p_Msg.transform.rotation.z);
                    Logger::Debug("position = ({}, {}, {})", p_Msg.transform.position.x, p_Msg.transform.position.y, p_Msg.transform.position.z);
                    // TODO
                },
                [this](const Qne::QnToSdkMessage_DeleteSpawnedEntity& p_Msg)
                {
                    Logger::Debug("QNE delete spawned entity {:x}", p_Msg.entity_id);
                    // TODO
                }
            }, s_Message);
        }
        catch (const std::exception& e)
        {
            Logger::Error("Failed to parse QNE message: {}", e.what());
            m_ConnectedToQne = false;
            return;
        }
    }
}

void Editor::SendQneMessage(const Qne::SdkToQnMessage& p_Message)
{
    Qne::MessageContainer s_MessageContainer {};
    s_MessageContainer.message_tag = static_cast<uint32_t>(p_Message.index());

    int s_MessageSize = sizeof(uint32_t);

    std::visit([&s_MessageSize, &s_MessageContainer](const auto& p_ActualMsg)
    {
        s_MessageSize += sizeof(p_ActualMsg);
        memcpy(s_MessageContainer.buf, &p_ActualMsg, sizeof(p_ActualMsg));
    }, p_Message);

    Logger::Debug("Sending QNE message {} (size {})", p_Message.index(), s_MessageSize);

    if (sendto(m_QneSocket, reinterpret_cast<const char*>(&s_MessageContainer), s_MessageSize, 0, reinterpret_cast<sockaddr*>(&m_QneAddress), sizeof(m_QneAddress)) == SOCKET_ERROR)
    {
        Logger::Error("Failed to send QNE message: {}", WSAGetLastError());
        m_ConnectedToQne = false;
    }
}


