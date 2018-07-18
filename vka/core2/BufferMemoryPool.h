#pragma once
#ifndef VKA_BUFFER_MEMORYPOOL_H
#define VKA_BUFFER_MEMORYPOOL_H

#include <vulkan/vulkan.hpp>

#include "Memory.h"

#include <vka/core/log.h>

namespace vka
{

class BufferMemoryPool : public context_child
{
    public:

    BufferMemoryPool(context * parent) : context_child(parent) ,
                                         m_memory(parent)
    {

    }

    ~BufferMemoryPool()
    {
        Destroy();
    }

    BufferMemoryPool* SetUsage(vk::BufferUsageFlags flags)
    {
        m_create_info.usage = flags;
        return this;
    }

    BufferMemoryPool* SetSize(std::size_t size)
    {
        m_create_info.size = size;
        return this;
    }

    BufferMemoryPool* SetMemoryProperties(vk::MemoryPropertyFlags flags)
    {
        m_memory.SetMemoryProperties(flags);
        return this;
    }


    void Destroy()
    {
        auto device = get_device();
        if(m_buffer)
        {
            device.destroyBuffer(m_buffer);
            m_buffer = vk::Buffer();
            LOG << "BufferMemoryPool destroyed" << ENDL;
        }
    }

    void Create()
    {
        auto device = get_device();

        if( m_create_info.size == 0)
        {
            throw std::runtime_error("Size not set! Please use the set_size( ) method.");
        }
        if( m_buffer )
        {
            throw std::runtime_error("Buffer already created");
        }


        m_buffer = device.createBuffer( m_create_info );

        if(!m_buffer)
            throw std::runtime_error("Failed to create buffer");

        m_memory.Allocate( device.getBufferMemoryRequirements(m_buffer) );

        m_create_info.size = m_memory.GetSize();
        m_memory.Bind(m_buffer, 0);

    }

protected:
    vk::Buffer  m_buffer;
    vka::Memory m_memory;

    vk::BufferCreateInfo    m_create_info;

};


}

#endif
