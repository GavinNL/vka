#ifndef VKA_BUFFER_POOL_H
#define VKA_BUFFER_POOL_H

#include <vka/core/buffer.h>
#include <vka/utils/buffer_memory_manager.h>

#include <set>

namespace vka
{

class buffer_pool;

class sub_buffer
{
public:
    operator vk::Buffer () const
    {
        return m_buffer;
    }


    void copy(void * data, vk::DeviceSize size, vk::DeviceSize offset);

protected:

    sub_buffer(){}
    ~sub_buffer() {}

    vk::Buffer     m_buffer;
    vk::DeviceSize m_size;
    vk::DeviceSize m_offset; // offset from teh start of the parent' memory block

    buffer_pool    *m_parent;
    friend class buffer_pool;
};

class buffer_pool : public buffer
{
public:

    buffer_pool* set_size(size_t s)
    {
        this->buffer::set_size(s);
        m_manager.reset(s);
        return this;
    }

    sub_buffer* new_buffer(size_t n);
    void free_buffer(sub_buffer * b);

protected:
    vka::buffer_memory_manager m_manager;
    std::set<sub_buffer*> m_sub_buffers;

    ~buffer_pool();

    buffer_pool(context * parent ) : buffer(parent)
    {
        this->set_usage(   vk::BufferUsageFlagBits::eIndexBuffer
                         | vk::BufferUsageFlagBits::eVertexBuffer
                         | vk::BufferUsageFlagBits::eUniformBuffer
                         | vk::BufferUsageFlagBits::eTransferDst
                           );
        this->set_memory_properties( vk::MemoryPropertyFlagBits::eDeviceLocal );
    }

    friend class context;
    friend class deleter<buffer_pool>;
};


}


#endif
