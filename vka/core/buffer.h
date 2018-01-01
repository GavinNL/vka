#ifndef VKA_BUFFER_H
#define VKA_BUFFER_H

#include <vulkan/vulkan.hpp>
#include "array_view.h"
#include "deleter.h"

namespace vka
{

class context;

class buffer
{
private:
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
        m_memory_type = flags;
        return this;
    }

    bool create();


    //==========================================================================
    size_t size() const
    {
        return m_create_info.size;
    }
    //==========================================================================

    template<typename T>
    array_view<T> map(size_t alignment=sizeof(T))
    {
        return array_view<T>(size(), map_memory(), alignment );
    }

    void * map_memory();
    void   unmap_memory();

protected:
    context * m_parent_context;


    vk::Buffer              m_buffer;
    vk::BufferCreateInfo    m_create_info;
    vk::MemoryPropertyFlags m_memory_type;
    vk::DeviceMemory        m_device_memory;


    void * m_mapped = nullptr;
    uint32_t findMemoryType(uint32_t typeFilter,
                            vk::MemoryPropertyFlags properties,
                            vk::Device device,
                            vk::PhysicalDevice physicaldevice);

    friend class context;
    friend class deleter<buffer>;


};


}

#endif
