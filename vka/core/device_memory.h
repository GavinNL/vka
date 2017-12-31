#ifndef VKA_DEVICE_MEMORY
#define VKA_DEVICE_MEMORY

#include <vulkan/vulkan.hpp>

namespace vka
{

class context;

class device_memory
{
    public:

    void create()
    {
        vk::MemoryAllocateInfo info;

    }


    private:
        context * m_parent_context;

        vk::DeviceMemory m_memory;
    friend class context;
};


}

#endif
