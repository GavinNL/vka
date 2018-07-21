#pragma once
#ifndef VKA_BUFFER_MEMORYPOOL_H
#define VKA_BUFFER_MEMORYPOOL_H

#include <vulkan/vulkan.hpp>

#include "Memory.h"

#include <vka/core/log.h>

#include <vka/utils/buffer_memory_manager.h>

namespace vka
{

class BufferMemoryPool;

/**
 * @brief The SubBuffer class
 *
 * A subbuffer represents a portion of the BufferMemoryPool
 *
 */
class SubBuffer
{
private:
    SubBuffer()
    {
    }
    public:

    ~SubBuffer()
    {
        Destroy();
    }

    SubBuffer(SubBuffer & other) = delete;
    SubBuffer & operator=(SubBuffer & other) = delete;

    vk::DeviceSize GetOffset() const
    {
        return m_offset;
    }

    vk::DeviceSize GetSize() const
    {
        return m_size;
    }

    void Destroy();

    vk::Buffer GetParentBufferHandle() const;

    void* MapBuffer();
    void  UnmapBuffer();

    void CopyData( void const * src, vk::DeviceSize d)
    {
        void * dst = MapBuffer();
        memcpy( dst, src, d );
        UnmapBuffer();
    }

    protected:
        BufferMemoryPool *    m_parent = nullptr;
        vk::DeviceSize        m_offset=0;
        vk::DeviceSize        m_size=0;

        friend class BufferMemoryPool;
};




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
        m_manager.reset( m_create_info.size  );

    }

    /**
     * @brief NewSubBuffer
     * @param size
     * @return
     *
     * Allocate a new SubBuffer from the buffer pool
     */
    std::shared_ptr<SubBuffer> NewSubBuffer(vk::DeviceSize size)
    {
        assert( size <= m_memory.GetSize() );
        auto offset = m_manager.allocate( size, m_memory.GetAlignment() );

        assert( offset < m_manager.error );
        auto S = std::shared_ptr<SubBuffer>( new SubBuffer() );
        S->m_parent = this;
        S->m_offset = offset;
        S->m_size   = size;
        return S;
    }

    void FreeSubBuffer( SubBuffer & S )
    {
        m_manager.free( S.m_offset);
        S.m_offset = 0;
        S.m_size = 0;
        S.m_parent = nullptr;
    }

    vk::Buffer GetBufferHandle() const
    {
        return m_buffer;
    }

    void * MapBuffer(vk::DeviceSize offset = 0)
    {
        uint8_t * d = (uint8_t*)m_memory.Map() + offset;

        return d;
    }

    void UnmapBuffer()
    {
        m_memory.UnMap();;
    }

protected:
    vk::Buffer                 m_buffer;
    vka::Memory                m_memory;
    vka::buffer_memory_manager m_manager;
    vk::BufferCreateInfo    m_create_info;

};

inline void SubBuffer::Destroy()
{
    if(m_parent)
        m_parent->FreeSubBuffer(*this);
}

inline vk::Buffer SubBuffer::GetParentBufferHandle() const
{
    return m_parent->GetBufferHandle();
}

inline void *SubBuffer::MapBuffer()
{
    return m_parent->MapBuffer(m_offset);
}

inline void SubBuffer::UnmapBuffer()
{
    m_parent->UnmapBuffer();
}

}

#endif
