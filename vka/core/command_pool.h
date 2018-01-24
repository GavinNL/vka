#ifndef VKA_COMMAND_POOL_H
#define VKA_COMMAND_POOL_H

#include <vulkan/vulkan.hpp>
#include "context_child.h"

#include <vka/core/command_buffer.h>

namespace vka
{

class context;

class command_pool : public context_child
{
public:
    CONTEXT_CHILD_DEFAULT_CONSTRUCTOR(command_pool)

    ~command_pool();


    operator vk::CommandPool() const
    {
        return m_command_pool;
    }

    vka::command_buffer AllocateCommandBuffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);

    void FreeCommandBuffer(vk::CommandBuffer cmd);
private:
    vk::CommandPool m_command_pool;

    friend class context;
};

}

#endif
