#include <vulkan/vulkan.hpp>
#include <vka/core/command_pool.h>
#include <vka/core/context.h>

vka::command_pool::~command_pool()
{
    if( m_parent_context)
    {
        m_parent_context->get_device().destroyCommandPool(m_command_pool);
    }
}

void vka::command_pool::FreeCommandBuffer(vk::CommandBuffer cmd)
{
    m_parent_context->get_device( ).freeCommandBuffers( m_command_pool,1, &cmd);
}

vk::CommandBuffer vka::command_pool::AllocateCommandBuffer(vk::CommandBufferLevel level)
{

    vk::CommandBufferAllocateInfo allocInfo;

    allocInfo.level              = level;
    allocInfo.commandPool        = m_command_pool;
    allocInfo.commandBufferCount = 1;

    std::vector<vk::CommandBuffer> commandBuffer_v = m_parent_context->get_device().allocateCommandBuffers( allocInfo );

    if( commandBuffer_v.size() == 0 )
        throw std::runtime_error("Error creating command buffers");


    return commandBuffer_v[0];
}
