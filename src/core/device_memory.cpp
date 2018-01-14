#include <vka/core/device_memory.h>
#include <vka/core/buffer.h>

vka::device_memory::~device_memory()
{
    free();
}

bool vka::device_memory::allocate(vka::buffer * b)
{
    auto req = get_device().getBufferMemoryRequirements(b->get());
    return allocate(req);

}

void vka::device_memory::free()
{
    if(m_memory)
    {
        if(m_mapped)
        {
            unmap();
        }
        get_device().freeMemory(m_memory);
        LOG << "Memory freed" << ENDL;
    }
}


bool vka::device_memory::allocate(vk::MemoryRequirements requirements)
{
    m_info.allocationSize  = requirements.size;
    m_info.memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, m_memory_properties);

    LOG << "Allocated Buffer Memory: Size: " <<requirements.size << "  alignment: " << requirements.alignment << ENDL;
    m_memory = get_device().allocateMemory(m_info);

    if( !m_memory)
    {
        throw std::runtime_error("Failed to allocate memory for buffer");
    }

    return true;
}

uint32_t vka::device_memory::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    auto memProperites        = get_physical_device().getMemoryProperties();
    for (uint32_t i = 0; i < memProperites.memoryTypeCount ; i++)
    {
        if ((typeFilter & (1 << i)) && ( static_cast<vk::MemoryPropertyFlags>(memProperites.memoryTypes[i].propertyFlags) & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}


void vka::buffer_memory::bind( vka::buffer * b, vk::DeviceSize memoryOffset )
{
    get_device().bindBufferMemory( *b , m_memory, memoryOffset);
}
