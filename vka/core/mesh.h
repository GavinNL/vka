#ifndef VKA_MESH_H
#define VKA_MESH_H

#include <vector>
#include <string.h>
#include <vulkan/vulkan.hpp>
#include <map>
#include <vka/core/array_view.h>

namespace vka
{

enum class VertexAttribute
{
    ePosition,
    eUV,
    eNormal,
    eColor
};

class mesh_t
{
    public:
        struct attr_info
        {
            uint32_t   offset;
            uint32_t   size;
            vk::Format format;
            bool operator==(const attr_info & other) const
            {
                return other.offset==offset && size==other.size && format==other.format;
            }

        };

        template<typename T>
        size_t push_back(T v)
        {
            m_data.insert( m_data.end(), sizeof(v) ,0);
            memcpy( &m_data.back() - sizeof(v) + 1, &v, sizeof(v));
            return m_data.size();
        }

        uint32_t num_indices() const
        {
            return index_data_size() / index_size();
        }
        uint32_t num_attributes() const
        {
            return m_attributes.size();
        }

        uint32_t vertex_size()    const
        {
            uint32_t s = 0;
            for(auto const & a : m_attributes)
                s += a.second.size;
            return s;
        }

        uint32_t index_size() const
        {
            return  index_type()==vk::IndexType::eUint16 ? sizeof(uint16_t) : sizeof(uint32_t);
        }

        bool has(VertexAttribute a) const
        {
            return m_attributes.count(a)==1;
        }

        void clear()
        {
            m_data.clear();
            m_attributes.clear();
        }

        void add_attribute(VertexAttribute a, vk::Format f, uint32_t s)
        {
            if(m_data.size())
            {
                throw std::runtime_error("Cannot add new attributes after vertex is inserted. Must clear the mesh first");
            }
            m_attributes[a].format = f;
            m_attributes[a].size   = s;
            update();
        }

        void update()
        {
            uint32_t s = 0;
            for(auto  & a : m_attributes)
            {
                a.second.offset = s;
                s += a.second.size;
            }
            m_vertex_size = s;
        }

        uint32_t offset(VertexAttribute a)
        {
            return m_attributes.at(a).offset;
        }
        vk::Format format(VertexAttribute a)
        {
            return m_attributes.at(a).format;
        }
        uint32_t size(VertexAttribute a)
        {
            return m_attributes.at(a).size;
        }

        void reserve_indices(size_t num_indices)
        {
            m_index.resize( num_indices * index_size() );
        }
        void reserve_vertices(size_t num_vertices)
        {
            if(num_attributes()==0)
                throw std::runtime_error("Must add attributes before you can reserve space");

            m_data.resize( num_vertices*vertex_size() );
        }

        size_t num_vertices() const
        {
            return m_data.size() / vertex_size();
        }

        template<typename T>
        vka::array_view<T> get_attribute_view( VertexAttribute a)
        {
            if(m_vertex_size==0) throw std::runtime_error("vertex size =0");
            if(sizeof(T) != size(a) ) throw std::runtime_error("Template size and attribute size are not the same");
            if(m_data.size() == 0) throw std::runtime_error("You must reserve vertices before calling get_view");

            uint8_t * c = m_data.data();
                     c += offset(a);
            return vka::array_view<T>(m_data.size()/m_vertex_size, c, m_vertex_size );
        }

        template<typename T>
        vka::array_view<T> get_index_view()
        {
            if( index_type() == vk::IndexType::eUint16)
            {
                if( sizeof(T) != sizeof(uint16_t) )
                    throw std::runtime_error("Incorrect template size. Does not match with index type");
            }
            if( index_type() == vk::IndexType::eUint32)
            {
                if( sizeof(T) != sizeof(uint32_t) )
                    throw std::runtime_error("Incorrect template size. Does not match with index type");
            }

            if(m_index.size() == 0) throw std::runtime_error("You must reserve indices before calling get_view");

            uint8_t * c = m_index.data();
            auto i_size = index_size();
            return vka::array_view<T>(m_index.size()/i_size, c);
        }

        void const* vertex_data() const
        {
            return m_data.data();
        }
        size_t const vertex_data_size() const
        {
            return m_data.size();
        }


        void const* index_data() const
        {
            return m_index.data();
        }
        size_t const index_data_size() const
        {
            return m_index.size();
        }

        void set_index_type(vk::IndexType t)
        {
            m_index_type = t;
        }
        vk::IndexType index_type() const
        {
            return m_index_type;
        }


        bool merge( mesh_t const & other )
        {
            if( other.num_attributes() != num_attributes() ) return false;
            if( other.index_type()     != index_type() ) return false;

            auto i = other.m_attributes.begin();
            for(auto & a : m_attributes)
            {
                if( !(i->second == a.second) )
                {
                    return false;
                }
                i++;
            }


            auto N = num_vertices();
            auto n = num_indices();

            std::copy(other.m_data.begin(), other.m_data.end(), std::back_inserter(m_data));
            std::copy(other.m_index.begin(), other.m_index.end(), std::back_inserter(m_index));

            if( m_index_type == vk::IndexType::eUint16)
            {
                auto I = get_index_view<uint16_t>();
                const auto s = num_indices();
                for(uint32_t j=n; j < s; j++)
                {
                    I[j] += N;
                }
            }
            else {
                auto I = get_index_view<uint32_t>();
                const auto s = num_indices();
                for(uint32_t j=n; j < s; j++)
                {
                    I[j] += N;
                }
            }

            return true;
        }


    private:
        std::vector<uint8_t>                 m_data;
        std::vector<uint8_t>                 m_index;
        vk::IndexType                        m_index_type=vk::IndexType::eUint16;
        std::map<VertexAttribute, attr_info> m_attributes;
        uint32_t m_vertex_size = 0;
};

}

#endif
