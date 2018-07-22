#pragma once
#ifndef VKA_MEMORY_H_H
#define VKA_MEMORY_H_H

#include <vulkan/vulkan.hpp>

#include <vka/core/context_child.h>

#include <vka/core/log.h>

namespace vka {


class context;

/**
 * @brief The MappedMemory class
 *
 * MappedMemory is a wrapper around a void* data type + extra handling to
 * unmap the memory from the host_visible vuklan memory.
 *
 * MappedMemory can be casted to void* so it can be passed into memcpy
 * It can be incremented using += / -=
 */
class MappedMemory
{
    public:
        MappedMemory(void * address, void * begin, uint32_t size, std::shared_ptr<int> unmapper_) :
            m_address((unsigned char*)address),
            m_begin(  (unsigned char*)begin),
            m_end(     m_begin + size ),
            m_shared(unmapper_)
        {
        }

        MappedMemory( MappedMemory const & M) :
            m_address(M.m_address),
            m_begin(  M.m_begin),
            m_end (   M.m_end),
            m_shared( M.m_shared)
        {
        }

        MappedMemory( MappedMemory && M)
        {
            m_address = M.m_address;         M.m_address = nullptr;
            m_begin    = M.m_begin;          M.m_begin    = nullptr;
            m_end      = M.m_end;            M.m_end      = nullptr;
            m_shared  = std::move(M.m_shared);
        }

        MappedMemory & operator = ( MappedMemory const & M)
        {
            if( &M != this)
            {
                m_address = M.m_address;
                m_begin    = M.m_begin;
                m_end      = M.m_end;
                m_shared  = std::move(M.m_shared);
            }
            return *this;
        }

        MappedMemory & operator = ( MappedMemory && M)
        {
            if( &M != this)
            {
                m_address = M.m_address;       M.m_address = nullptr;
                m_begin    = M.m_begin;          M.m_begin    = nullptr;
                m_end      = M.m_end;            M.m_end      = nullptr;
                m_shared  = std::move(M.m_shared);
            }
            return *this;
        }

        template<typename IntType>
        MappedMemory & operator += (IntType i)
        {
            m_address += i;
            assert( m_address < m_end);
            return *this;
        }
        template<typename IntType>
        MappedMemory & operator -= (IntType i)
        {
            m_address -= i;
            assert( m_address >= m_begin);
            return *this;
        }
        template<typename IntType>
        MappedMemory operator + (IntType i)
        {
            auto M = *this;
            M.m_address += i;
            assert( M.m_address >= M.m_begin);
            return M;
        }
        template<typename IntType>
        MappedMemory operator - (IntType i)
        {
            auto M = *this;
            M.m_address -= i;
            assert( M.m_address < M.m_end);
            return M;
        }

        operator void const *()
        {
            return (void const*)(m_address);
        }
        operator void*()
        {
            return (void*)(m_address);
        }
        operator unsigned char*()
        {
            return (unsigned char*)(m_address);
        }
        operator unsigned char const*()
        {
            return (unsigned char const*)(m_address);
        }

        template<typename T>
        typename std::enable_if<  std::is_pod<T>::value , MappedMemory >::type&
        operator << ( T const  & M)
        {
            memcpy(m_address, &M, sizeof(T));
            m_address += sizeof(T);
            return *this;
        }

        void Reset()
        {
            m_address = m_begin = m_end = nullptr;
            m_shared.reset();
        }

        unsigned char & operator[]( std::uint32_t i)
        {
            return m_address[i];
        }

        void memcpy( void const * srcData, vk::DeviceSize srcSize, vk::DeviceSize dstOffset=0)
        {
            assert( srcSize < size()-dstOffset);
            ::memcpy( m_address + dstOffset, srcData, srcSize);
        }

        vk::DeviceSize size() const
        {
            return m_end - m_begin;
        }

        unsigned char       *m_address;
        unsigned char       *m_begin;
        unsigned char       *m_end;
        std::shared_ptr<int> m_shared;
};


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

    MappedMemory GetMappedMemory(vk::DeviceSize offset=0)
    {
        if( m_data->m_memory_properties & vk::MemoryPropertyFlagBits::eHostVisible )
        {
            if(m_data->m_mapped)
            {
                m_data->m_mapped_count++;

                //-------------------------
                auto Unmapper = [this](int * i)
                {
                    delete i;
                    this->UnMap();
                };
                std::shared_ptr<int> Unmapper_( new int(),Unmapper);

                MappedMemory M( (unsigned char*)(m_data->m_mapped) + offset,
                                (unsigned char*)(m_data->m_mapped) + offset,
                                GetSize()-offset,
                                Unmapper_);
                //-------------------------

                return M;
            }

            void * data = get_device().mapMemory( m_memory, 0, GetSize(), vk::MemoryMapFlags());

            m_data->m_mapped_count++;
            m_data->m_mapped = data;


            //-------------------------
            auto Unmapper = [this](int * i)
            {
                delete i;
                this->UnMap();
            };
            std::shared_ptr<int> Unmapper_( new int(),Unmapper);
            MappedMemory M( (unsigned char*)data + offset,
                            (unsigned char*)data + offset,
                            GetSize()-offset,
                            Unmapper_);
            //-------------------------

            return M;
        }
        throw std::runtime_error("Device is not HostVisible. Cannot map data");
        //return nullptr;
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
