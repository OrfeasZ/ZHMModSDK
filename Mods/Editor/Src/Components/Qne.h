#pragma once

#include <cstdint>
#include <variant>
#include <concepts>

#pragma pack(push, 1)

namespace Qne
{
    constexpr inline uint32_t PROTOCOL_VERSION = 1;

    struct Vec3
    {
        float x = 0.f;
        float y = 0.f;
        float z = 0.f;
    };

    struct Transform
    {
        Vec3 rotation = {};
        Vec3 position = {};
    };

    struct SdkToQnMessage_Hello
    {
        uint32_t protocol_version = PROTOCOL_VERSION;
    };

    struct SdkToQnMessage_SelectEntity
    {
        uint64_t entity_id = 0;
        ZRuntimeResourceID tblu_hash = {};
    };

    struct SdkToQnMessage_SetEntityTransform
    {
        uint64_t entity_id = 0;
        ZRuntimeResourceID tblu_hash = {};
        Transform transform = {};
    };

    using SdkToQnMessage = std::variant<
        SdkToQnMessage_Hello,
        SdkToQnMessage_SelectEntity,
        SdkToQnMessage_SetEntityTransform
    >;


    struct QnToSdkMessage_Hello
    {
        uint32_t protocol_version = PROTOCOL_VERSION;
    };

    struct QnToSdkMessage_SelectEntity
    {
        uint64_t entity_id = 0;
        ZRuntimeResourceID tblu_hash = {};
    };

    struct QnToSdkMessage_SetEntityTransform
    {
        uint64_t entity_id = 0;
        ZRuntimeResourceID tblu_hash = {};
        Transform transform = {};
    };

    struct QnToSdkMessage_SpawnEntity
    {
        uint64_t entity_id = 0;
        ZRuntimeResourceID temp_hash = {};
    };

    struct QnToSdkMessage_SetSpawnedEntityTransform
    {
        uint64_t entity_id = 0;
        Transform transform = {};
    };

    struct QnToSdkMessage_DeleteSpawnedEntity
    {
        uint64_t entity_id = 0;
    };

    using QnToSdkMessage = std::variant<
        QnToSdkMessage_Hello,
        QnToSdkMessage_SelectEntity,
        QnToSdkMessage_SetEntityTransform,
        QnToSdkMessage_SpawnEntity,
        QnToSdkMessage_SetSpawnedEntityTransform,
        QnToSdkMessage_DeleteSpawnedEntity
    >;

    struct MessageContainer
    {
        uint32_t message_tag;
        uint8_t buf[max(sizeof(Qne::SdkToQnMessage), sizeof(Qne::QnToSdkMessage)) + 64];
    };

    template <class T>
    concept Variant = requires
    {
        std::variant_size_v<T>;
    };

    template <Variant T, std::size_t I = 0>
    inline T VariantFromIndexWithData(std::size_t p_Index, void* p_Data)
    {
        if constexpr (I >= std::variant_size_v<T>)
        {
            throw std::runtime_error("Index was out of bounds.");
        }
        else
        {
            if (p_Index != I)
                return VariantFromIndexWithData<T, I + 1>(p_Index, p_Data);

            return T(std::in_place_index<I>, *static_cast<std::variant_alternative_t<I, T>*>(p_Data));
        }
    }

    inline QnToSdkMessage ParseQnToSdkMessage(const MessageContainer& p_Container)
    {
        const auto s_VariantIndex = p_Container.message_tag;

        if (s_VariantIndex >= std::variant_size_v<QnToSdkMessage>)
        {
            throw std::runtime_error("Index was out of bounds.");
        }

        return VariantFromIndexWithData<QnToSdkMessage>(s_VariantIndex, (void*)(p_Container.buf));
    }

    template <class... Ts> struct Overload : Ts...
    {
        using Ts::operator()...;
    };
}

#pragma pack(pop)
