#ifndef VKA_COMMAND_POOL_H
#define VKA_COMMAND_POOL_H

#include <vulkan/vulkan.hpp>
#include "context_child.h"

namespace vka
{

class context;

class command_pool : public context_child
{
public:
    CONTEXT_CHILD_DEFAULT_CONSTRUCTOR(command_pool)

    ~command_pool();

    vk::CommandBuffer AllocateCommandBuffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);

    void FreeCommandBuffer(vk::CommandBuffer cmd);
private:
    vk::CommandPool m_command_pool;

    friend class context;
};

}

#endif
