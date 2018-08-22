#pragma once
#ifndef VKA_COMMANDPOOL2_H
#define VKA_COMMANDPOOL2_H

#include <vka/core/log.h>
#include <vka/core/command_buffer.h>
#include <vka/core/context_child.h>

namespace vka
{

class CommandPool : public context_child
{
public:
    CommandPool(context * parent) : context_child(parent)
    {
    }

    ~CommandPool()
    {
        if(m_command_pool)
        {
            get_device().destroyCommandPool(m_command_pool);
            m_command_pool = vk::CommandPool();
        }
    }

    void create();

    void freeCommandBuffer(vka::command_buffer cmd);

    vka::command_buffer allocateCommandBuffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary );
protected:
    vk::CommandPool m_command_pool;
};

}

#endif
