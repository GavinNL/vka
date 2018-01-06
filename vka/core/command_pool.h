#ifndef VKA_COMMAND_POOL_H
#define VKA_COMMAND_POOL_H

#include <vulkan/vulkan.hpp>

namespace vka
{

class context;

class command_pool
{
public:
    ~command_pool();

    vk::CommandBuffer AllocateCommandBuffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);

    void FreeCommandBuffer(vk::CommandBuffer cmd);
private:

    vk::CommandPool m_command_pool;

    context * m_parent_context = nullptr;

    friend class context;
};

}

#endif
