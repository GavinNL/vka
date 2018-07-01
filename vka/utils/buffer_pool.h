#ifndef VKA_BUFFER_POOL_H
#define VKA_BUFFER_POOL_H

#include <vka/core/buffer.h>
#include <vka/utils/buffer_memory_manager.h>

#include <set>


namespace vka
{

class buffer_pool;

struct sub_buffer_object
{
    vk::DeviceSize m_offset;
    vk::DeviceSize m_size;
};

/**
 * @brief The sub_buffer class
 *
 * A sub_buffer is an allocated region within a much larger buffer.
 *
 * The sub_buffer is a reference to the parent buffer, plus an offset
 */
class sub_buffer
{
public:
    operator vk::Buffer () const
    {
        return get();
    }

    vk::Buffer get() const
    {
        return m_DescriptorBufferInfo.buffer;
    }


    void copy(void const * data, vk::DeviceSize size, vk::DeviceSize _offset=0);


    vk::DeviceSize size() const
    {
        return m_DescriptorBufferInfo.range;
    }

    vk::DeviceSize offset() const
    {
        return m_DescriptorBufferInfo.offset;
    }

    vka::buffer_pool* get_parent_pool() {
        return m_parent;
    }

    vk::DescriptorBufferInfo & get_descriptor_info()
    {
        return m_DescriptorBufferInfo;
    }

protected:

    sub_buffer( buffer_pool * parent) : m_parent(parent){}
    ~sub_buffer() {}

    vk::DescriptorBufferInfo m_DescriptorBufferInfo;

    buffer_pool               *m_parent;

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

    /**
     * @brief new_buffer
     * @param n
     * @param alignment
     * @return
     *
     * Allocates data withiin the pool and returns new sub_buffer of size n.
     */
    sub_buffer* new_buffer(vk::DeviceSize n, vk::DeviceSize alignment=1);
    void        free_buffer(sub_buffer * b);

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
