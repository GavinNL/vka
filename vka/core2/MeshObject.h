#pragma once
#ifndef VKA_MESH_OBJECT_H
#define VKA_MESH_OBJECT_H

#include "BufferMemoryPool.h"

#include <map>


namespace vka
{

struct SubObject_t
{
    uint32_t index_count = 0;
    uint32_t first_index = 0;
    uint32_t vertex_offset = 0;
};



/**
 * @brief The MeshObject class
 *
 * A MeshObject is used to hold a full mesh with multiple attributes.
 */
class MeshObject
{
public:

    MeshObject() {}
    ~MeshObject() {}

    MeshObject( MeshObject const & other)
    {
        m_attributes    =    other.m_attributes;
        m_index_buffer  =    other.m_index_buffer;
        m_index_type    =    other.m_index_type;
        m_subObjects    =    other.m_subObjects;
    }

    MeshObject( MeshObject && other)
    {
        m_attributes    = std::move(     other.m_attributes );
        m_index_buffer  = std::move(     other.m_index_buffer );
        m_index_type    = std::move(     other.m_index_type );
        m_subObjects    = std::move(     other.m_subObjects );
    }

    MeshObject & operator = ( MeshObject const & other)
    {
        if( this != & other)
        {
            m_attributes     =    other.m_attributes;
            m_index_buffer   =    other.m_index_buffer;
            m_index_type     =    other.m_index_type;
            m_subObjects     =    other.m_subObjects;
        }
        return *this;
    }

    MeshObject & operator = ( MeshObject && other)
    {
        if( this != & other)
        {
            m_attributes    = std::move(     other.m_attributes );
            m_index_buffer  = std::move(     other.m_index_buffer );
            m_index_type    = std::move(     other.m_index_type );
            m_subObjects    = std::move(     other.m_subObjects );
        }
        return *this;
    }


    void AddIndexBuffer(vk::IndexType index_type, SubBuffer_p index_buffer)
    {
        m_index_buffer = index_buffer;
        m_index_type   = index_type;
    }

    void AddAttributeBuffer( uint32_t index, SubBuffer_p attr)
    {
        m_attributes[index] = attr;
    }

    SubBuffer_p const & GetAttributeBuffer(uint32_t index) const
    {
        return m_attributes.at(index);
    }

    SubBuffer_p const & GetIndexBuffer() const
    {
        return m_index_buffer;
    }

    SubBuffer_p & GetAttributeBuffer(uint32_t index)
    {
        return m_attributes.at(index);
    }

    SubBuffer_p & GetIndexBuffer()
    {
        return m_index_buffer;
    }

    vk::IndexType GetIndexType() const
    {
        return m_index_type;
    }

    std::map<uint32_t, SubBuffer_p > const & GetAttributeBuffers() const
    {
        return m_attributes;
    }
 protected:
    std::map<uint32_t, SubBuffer_p > m_attributes;
    SubBuffer_p                      m_index_buffer;
    vk::IndexType                    m_index_type;
    std::vector< SubObject_t >       m_subObjects;
};

}

#endif
