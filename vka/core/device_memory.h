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

        vk::DeviceMemory m_memory;
    friend class context;
};


}

#endif
