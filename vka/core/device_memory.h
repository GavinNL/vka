#ifndef VKA_DEVICE_MEMORY
#define VKA_DEVICE_MEMORY

#include <vulkan/vulkan.hpp>
#include "context_child.h"
#include "log.h"

namespace vka
{

class context;
class buffer;

class device_memory : public context_child
{
    public:

    device_memory* set_memory_properties( vk::MemoryPropertyFlags props)
    {
        m_memory_properties = props;
        return this;
    }
    vk::MemoryPropertyFlags get_memory_properties() const
    {
        return m_memory_properties;
    }


    operator bool()
    {
        return static_cast<bool>(m_memory);
    }

    operator vk::DeviceMemory()
    {
        return m_memory;
    }

    vk::DeviceSize size() const
    {
        return m_info.allocationSize;
    }


    bool allocate(vk::MemoryRequirements requirements);
    void free();

    void * map()
    {
        if( m_memory_properties & vk::MemoryPropertyFlagBits::eHostVisible )
        {
            if(m_mapped)
                return m_mapped;

            void * data = get_device().mapMemory( m_memory, 0, size(), vk::MemoryMapFlags());

            m_mapped = data;
            return m_mapped;
        }
        throw std::runtime_error("Device is not HostVisible. Cannot map data");
        return nullptr;
    }

    void   unmap()
    {
        if(m_mapped)
        {
            get_device().unmapMemory(m_memory);
            m_mapped = nullptr;
        }
    }


    protected:

    CONTEXT_CHILD_DEFAULT_CONSTRUCTOR(device_memory)

    ~device_memory();

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    vk::DeviceMemory        m_memory;
    vk::MemoryAllocateInfo  m_info;
    vk::MemoryPropertyFlags m_memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
    void*                   m_mapped = nullptr;
    friend class deleter<context>;
    friend class buffer;
};

class buffer_memory : public device_memory
{
public:
    buffer_memory(context * p) : device_memory(p)
    {}

    void bind( vka::buffer * b, vk::DeviceSize memoryOffset=0);
};

class image_memory : public device_memory
{
    image_memory(context * p) : device_memory(p)
    {}


};


}

#endif
