#ifndef VKA_BUFFER_H
#define VKA_BUFFER_H

#include <vulkan/vulkan.hpp>
#include "context_child.h"
#include "array_view.h"
#include "deleter.h"
#include "device_memory.h"

namespace vka
{

class context;


class buffer : public context_child
{
protected:
    buffer( context* parent) : context_child(parent)
                              ,m_memory(parent){}
    ~buffer();
public:


    buffer* set_usage(vk::BufferUsageFlags flags)
    {
        m_create_info.usage = flags;
        return this;
    }

    buffer* set_size(std::size_t size)
    {
        m_create_info.size = size;
        return this;
    }

    buffer* set_memory_properties(vk::MemoryPropertyFlags flags)
    {
        m_memory.set_memory_properties(flags);
        return this;
    }

    bool create();

    operator vk::Buffer () const
    {
        return m_buffer;
    }

    vk::Buffer get()
    {
        return m_buffer;
    }
    //==========================================================================
    size_t size() const
    {
        return m_create_info.size;
    }
    //==========================================================================

    template<typename T>
    array_view<T> map(size_t byte_offset=0, size_t alignment=sizeof(T))
    {
        unsigned char * v = reinterpret_cast<unsigned char*>(map_memory());
        return array_view<T>(size()/sizeof(T), v+byte_offset, alignment );
    }

    void * map_memory();
    void   unmap_memory();

    /**
     * @brief copy
     * @param data
     * @param size
     * @param offset
     * @return
     *
     * Copy data into the buffer. If this buffer is host_visible.
     * it will map the buffer, copy the data, and then unmap it.
     *
     * If the buffer is not visible to the host it will copy the data
     * to a staging buffer, then copy it to the buffer. It will allocate
     * construct and free the command buffer needed
     */
    bool copy(void const * data, vk::DeviceSize size, vk::DeviceSize offset);
protected:

    vk::Buffer              m_buffer;
    vk::BufferCreateInfo    m_create_info;

    vka::buffer_memory      m_memory;

    friend class context;
    friend class deleter<buffer>;


};


}

#endif
