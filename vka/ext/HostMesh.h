#ifndef VKA_MESH_H
#define VKA_MESH_H

#include <vector>
#include <string.h>
#include <map>


#include <vulkan/vulkan.hpp>
#include <vka/core/array_view.h>
#include <vka/core/types.h>

namespace vka
{

enum class VertexAttribute
{
    ePosition,
    eUV,
    eNormal,
    eColor,
    eIndex
};


class host_mesh
{
public:
    struct AttributeInfo_t
    {
        vk::Format           m_format;
        uint32_t             m_AttributeSize; // size in bytes of the attribute data
        std::vector<uint8_t> m_data;

        /**
         * @brief attribute_size
         * @return
         * Returns the number of bytes a single attribute takes
         */
        uint32_t attribute_size() const
        {
            return m_AttributeSize;//return format_size( m_format );
        }

        /**
         * @brief format
         * @return
         * Returns the Vulkan format of the attribute
         */
        vk::Format format() const
        {
            return m_format;
        }

        /**
         * @brief data
         * @return
         * Returns a pointer to the raw data
         */
        void const * data() const
        {
            return m_data.data();
        }

        /**
         * @brief data_size
         * @return
         * Returns the total number of bytes all the attributes take up
         */
        size_t data_size() const
        {
            return m_data.size();
        }

        /**
         * @brief count
         * @return
         * Returns the total number of attributes.
         */
        size_t count() const
        {
            return m_data.size() / attribute_size();
        }

        template<typename T>
        array_view<T> view()
        {
            auto m_vertex_size = attribute_size();

            if(m_vertex_size==0) throw std::runtime_error("vertex size =0");
            if(sizeof(T) != m_vertex_size ) throw std::runtime_error("Template size and attribute size are not the same");
            if(m_data.size() == 0) throw std::runtime_error("You must reserve vertices before calling get_view");

            return array_view<T>( m_data.size()/m_vertex_size, m_data.data(), m_vertex_size );
        }

        template<typename T>
        array_view<const T> view() const
        {
            auto m_vertex_size = attribute_size();

            if(m_vertex_size==0) throw std::runtime_error("vertex size =0");
            if(sizeof(T) != m_vertex_size ) throw std::runtime_error("Template size and attribute size are not the same");
            if(m_data.size() == 0) throw std::runtime_error("You must reserve vertices before calling get_view");

            return array_view<const T>( m_data.size()/m_vertex_size, m_data.data(), m_vertex_size );
        }

        template<typename T>
        /**
         * @brief push_back
         * @param v
         * Pushes an attribute to the end of the buffer.
         *
         * This is a template function, a
         */
        void push_back(T const v)
        {
            assert(sizeof(T)==m_AttributeSize);
            m_data.insert( m_data.end(), sizeof(T) ,0);
            memcpy( &m_data.back() - sizeof(T) + 1, &v, sizeof(T));
        }
    };

    template<typename T>
    size_t push_back(VertexAttribute A, T const v)
    {
        auto & m_data = m_attributes[A].m_data;

        m_data.insert( m_data.end(), sizeof(T) ,0);
        memcpy( &m_data.back() - sizeof(T) + 1, &v, sizeof(v));
        return m_data.size();
    }


    void set_format(VertexAttribute A, vk::Format format, uint32_t attribute_size_in_bytes)
    {
        m_attributes[A].m_format = format;
        m_attributes[A].m_AttributeSize = attribute_size_in_bytes;
    }


    AttributeInfo_t & get_attribute(VertexAttribute A)
    {
        return m_attributes[A];
    }
    bool has_attribute(VertexAttribute A) const
    {
        return m_attributes.count(A)!=0;
    }

    std::map< VertexAttribute , AttributeInfo_t > const & get_map() const
    {
        return m_attributes;
    }

    std::map< VertexAttribute , AttributeInfo_t > m_attributes;

};

}

#endif
