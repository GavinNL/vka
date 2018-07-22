#pragma once
#ifndef VKA_MEMORY_H_H
#define VKA_MEMORY_H_H

#include <vulkan/vulkan.hpp>

#include <vka/core/context_child.h>

#include <vka/core/log.h>

namespace vka {


class context;


class Memory : public context_child
{
    public:

    struct Data_t
    {
        vk::MemoryAllocateInfo  m_info;
        vk::MemoryPropertyFlags m_memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
        vk::MemoryRequirements  m_memory_req;
        int32_t                 m_mapped_count = 0;
        void*                   m_mapped = nullptr;
    };

    Memory(context * parent) : context_child(parent)
    {
        m_data.reset( new Data_t() );
    }

    ~Memory()
    {
        Free();
    }

    Memory* SetMemoryProperties( vk::MemoryPropertyFlags props)
    {
        m_data->m_memory_properties = props;
        return this;
    }
    vk::MemoryPropertyFlags GetMemoryProperties() const
    {
        return m_data->m_memory_properties;
    }


    operator bool()
    {
        return static_cast<bool>(m_memory);
    }

    operator vk::DeviceMemory()
    {
        return m_memory;
    }

    vk::DeviceSize GetSize() const
    {
        return m_data->m_info.allocationSize;
    }

    vk::MemoryRequirements GetMemoryRequirements() const
    {
        return m_data->m_memory_req;
    }

    void Bind( vk::Buffer b, vk::DeviceSize memoryOffset )
    {
        get_device().bindBufferMemory( b , m_memory, memoryOffset);
    }
    void Bind( vk::Image img, vk::DeviceSize memoryOffset )
    {
        get_device().bindImageMemory( img , m_memory, memoryOffset);
    }

    bool Allocate(vk::MemoryRequirements requirements)
    {
        LOG << "Memory: Allocating: " << requirements.size << " bytes " <<ENDL;
        m_data->m_info.allocationSize  = requirements.size;
        m_data->m_info.memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, m_data->m_memory_properties);

        m_memory = get_device().allocateMemory( m_data->m_info );

        if( !m_memory)
        {
            throw std::runtime_error("Failed to allocate memory for buffer");
        }
        m_data->m_memory_req = requirements;
        return true;
    }

    vk::DeviceSize GetAlignment() const
    {
        return m_data->m_memory_req.alignment;
    }

    void Free()
    {
        if(m_memory)
        {
            assert( m_data->m_mapped_count==0);
            if(IsMapped())
            {
                UnMap();
            }
            get_device().freeMemory(m_memory);
            m_memory = vk::DeviceMemory();
            m_data->m_mapped = nullptr;
            m_data->m_mapped_count = 0;
            LOG << "Memory Freed" << ENDL;
        }
    }

    bool IsMapped()
    {
        if(m_data->m_mapped)
            return true;
        return false;
    }

    void * Map()
    {
        if( m_data->m_memory_properties & vk::MemoryPropertyFlagBits::eHostVisible )
        {
            if(m_data->m_mapped)
            {
                m_data->m_mapped_count++;
                return m_data->m_mapped;
            }

            void * data = get_device().mapMemory( m_memory, 0, GetSize(), vk::MemoryMapFlags());

            m_data->m_mapped_count++;
            m_data->m_mapped = data;
            return data;
        }
        throw std::runtime_error("Device is not HostVisible. Cannot map data");
        return nullptr;
    }

    void   UnMap()
    {
        if(m_data->m_mapped)
        {
            m_data->m_mapped_count--;

            if( m_data->m_mapped_count==0)
            {
                get_device().unmapMemory(m_memory);
                m_data->m_mapped = nullptr;
            }
        }
    }


    protected:



    vk::DeviceMemory          m_memory;
    std::unique_ptr<Data_t>   m_data;

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
    {
        auto memProperites        = get_physical_device().getMemoryProperties();

        for (uint32_t i = 0; i < memProperites.memoryTypeCount ; i++)
        {
            const vk::MemoryPropertyFlags MemPropFlags = static_cast<vk::MemoryPropertyFlags>(memProperites.memoryTypes[i].propertyFlags);
            #define GET_BIT(a, j) (a & (1 << j))

            if ( GET_BIT(typeFilter,i) && ( MemPropFlags & properties) == properties)
            {
                return i;
            }
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }

};


}

#endif
