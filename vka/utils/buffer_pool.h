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

class sub_buffer
{
public:
    operator vk::Buffer () const
    {
        return m_buffer;
    }

    vk::Buffer get() const
    {
        return m_buffer;
    }


    sub_buffer_object reserve(vk::DeviceSize size, vk::DeviceSize alignment=1);

    /**
     * @brief copy
     * @param data
     * @param size
     * @return
     *
     * Inserts data into this buffer where there is space. and returns
     * a sub_bufffer_object. which indicates where in the buffer
     * this object is. You must keep track of
     */
    sub_buffer_object insert(void const * data, vk::DeviceSize size, vk::DeviceSize alignment=1);

    /**
     * @brief free_buffer_boject
     * @param obj
     *
     * Reclaim the space used by that buffer object.
     */
    void free_buffer_boject( const sub_buffer_object & obj);


    /**
     * @brief clear_buffer_objects
     * Clears all buffer objects that the sub buffer is keeping track of.
     * The data is still there, but any allocation information about the
     * blocks are gone.
     */
    void clear_buffer_objects();


    vk::DeviceSize size() const
    {
        return m_size;
    }

    vk::DeviceSize offset() const
    {
        return m_offset;
    }
protected:

    sub_buffer( buffer_pool * parent) : m_parent(parent){}
    ~sub_buffer() {}

    vk::Buffer     m_buffer;

    vk::DeviceSize m_size;
    vk::DeviceSize m_offset; // offset from teh start of the parent' memory block

    buffer_pool    *m_parent;
    vka::buffer_memory_manager m_manager;

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
