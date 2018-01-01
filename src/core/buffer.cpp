#include <vka/core/buffer.h>
#include <vka/core/device_memory.h>
#include <vka/core/context.h>
#include <vka/core/array_view.h>


vka::buffer::~buffer()
{
    if( m_parent_context )
    {
        if(m_mapped)
        {
            unmap_memory();
        }

        if(m_device_memory)
        {
            LOG << "Memory freed"             << ENDL;
            m_parent_context->get_device().freeMemory(m_device_memory);
        }

        if(m_buffer)
        {
            m_parent_context->get_device().destroyBuffer(m_buffer);
        }
        LOG << "Buffer destroyed " << ENDL;
    }
}

bool vka::buffer::create()
{
    auto device = m_parent_context->get_device();
    auto physical_device = m_parent_context->get_physical_device();

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

    //==================== Allocate Memory =======================
    LOG << "Allocating buffer memory" << ENDL;
    vk::MemoryAllocateInfo allocInfo;

    auto memRequirements      = device.getBufferMemoryRequirements( m_buffer);

    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType( memRequirements.memoryTypeBits, m_memory_type ,device, physical_device);

    m_device_memory =  device.allocateMemory( allocInfo );

    if( !m_device_memory )
    {
        throw std::runtime_error("Failed to allocate memory for buffer");
    }

    m_create_info.size = memRequirements.size;

    device.bindBufferMemory(m_buffer, m_device_memory, 0);
    LOG << "Buffer Memory Allocated: " << m_create_info.size << " bytes" << ENDL;
    return true;
}


uint32_t vka::buffer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, vk::Device device, vk::PhysicalDevice physicaldevice)
{
   // auto memRequirements      = device.getBufferMemoryRequirements( m_buffer );
    auto memProperites        = physicaldevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProperites.memoryTypeCount ; i++)
    {
        // vk::MemoryPropertyFlagBits properties = static_cast<vk::MemoryPropertyFlagBits>(Properties);
        if ((typeFilter & (1 << i)) && ( static_cast<vk::MemoryPropertyFlags>(memProperites.memoryTypes[i].propertyFlags) & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}


void * vka::buffer::map_memory()
{

    if( m_memory_type & vk::MemoryPropertyFlagBits::eHostVisible )
    {
        void * data = m_parent_context->get_device().mapMemory( m_device_memory, 0, m_create_info.size, vk::MemoryMapFlags());
        m_mapped = data;
        return m_mapped;
    }
    return nullptr;
}

void   vka::buffer::unmap_memory()
{
    if(m_mapped)
    {
        m_parent_context->get_device().unmapMemory(m_device_memory);
        m_mapped = nullptr;
    }
}
