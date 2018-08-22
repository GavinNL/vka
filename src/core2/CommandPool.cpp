#include <vka/core2/CommandPool.h>
#include <vka/core/context.h>

namespace vka
{

void vka::CommandPool::create()
{
    vk::CommandPoolCreateInfo poolInfo;

    poolInfo.queueFamilyIndex = get_parent_context()->getQueueFamily().graphics;
    poolInfo.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer; // Optional

    m_command_pool = get_device().createCommandPool(poolInfo);

    if( !m_command_pool )
    {
        throw std::runtime_error("Failed to create command pool!");
    }
    LOG << "Command Pool created" << ENDL;
}

command_buffer CommandPool::allocateCommandBuffer(vk::CommandBufferLevel level)
{
    vk::CommandBufferAllocateInfo allocInfo;

    allocInfo.level              = level;
    allocInfo.commandPool        = m_command_pool;
    allocInfo.commandBufferCount = 1;

    std::vector<vk::CommandBuffer> commandBuffer_v = get_device().allocateCommandBuffers( allocInfo );

    if( commandBuffer_v.size() == 0 )
        throw std::runtime_error("Error creating command buffers");


    return commandBuffer_v[0];
}

void CommandPool::freeCommandBuffer(vka::command_buffer cmd)
{
     get_device().freeCommandBuffers( m_command_pool, 1, &cmd);
}

}
