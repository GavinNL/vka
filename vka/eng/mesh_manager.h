#include "manager_base.h"
#include <vka/core/context.h>
#include <vka/utils/buffer_pool.h>
#include <vka/core/command_buffer.h>

namespace vka
{

class mesh_manager;

class mesh
{
#define MAX_ATTRIBUTES 10
    struct attribute_data_t
    {
        vka::sub_buffer* buffer = nullptr;
        uint32_t         vertex_size = 0;
    };

    struct submesh_info_t
    {
        uint32_t    m_vertex_offset = 0;
        uint32_t    m_index_offset  = 0;
        uint32_t    m_index_count   = 0;
    };

   public:
    mesh(mesh_manager *m) : m_manager(m)
    {
    }

    ~mesh()
    {

    }

    uint32_t num_attributes() const
    {
        return m_num_attributes;
    }

    void set_num_vertices(uint32_t i)
    {
        m_vertex_count = i;
    }

    void set_num_indices(uint32_t i, uint32_t index_size)
    {
   //     m_submesh_info.m_index_count = i;
        m_index_count = i;
        m_index_size = index_size;
    }

    void set_attribute(uint32_t index, uint32_t attribute_size)
    {
        m_attributes[index].vertex_size = attribute_size;
    }

    bool allocate();


    /**
     * @brief copy_data
     * @param attribute_index
     * @param data
     * @param byte_size
     *
     * Copies data into the vertex attribute
     */
    void copy_attribute_data(uint32_t attribute_index, void const * data, uint32_t byte_size)
    {
        m_attributes[attribute_index].buffer->insert(data,byte_size, m_attributes[attribute_index].vertex_size);
    }


    void copy_index_data(void const * data, uint32_t byte_size)
    {
        m_index_buffer->insert(data, byte_size);
    }


    //====================================== Drawing Commands =================
    void bind(vka::command_buffer & cb)
    {
        if( m_vertex_count )
        {
            uint32_t i = 0;
            if( m_index_buffer)
            {
                cb.bindIndexSubBuffer(m_index_buffer, vk::IndexType::eUint16);
            }

            for(auto & b : m_attributes)
            {
                if(b.buffer==nullptr) return;
                    cb.bindVertexSubBuffer( i++, b.buffer);

            }
        }
    }

    void draw( vka::command_buffer & cb, uint32_t instance_count=1,uint32_t first_instance = 0)
    {
        if( m_vertex_count )
        {
            if( m_index_buffer)
                cb.drawIndexed(m_index_count, instance_count , 0, 0, first_instance);
            else
                cb.draw(m_vertex_count , instance_count, 0 , first_instance);
        }
    }
   private:

       mesh_manager                                * m_manager = nullptr;
       std::array< attribute_data_t, MAX_ATTRIBUTES> m_attributes;
       vka::sub_buffer                             * m_index_buffer = nullptr;
       uint32_t                                      m_num_attributes=0;

       uint32_t                                      m_index_count = 0;
       uint32_t                                      m_index_size = 0;
       uint32_t                                      m_vertex_count = 0;
       //submesh_info_t                                m_submesh_info;

       friend class mesh_manager;

};

class mesh_manager : public manager_base
{
    public:
        mesh_manager& set_buffer_size(size_t s)
        {
            create_buffer_pool(s);
            return *this;
        }

        void create_buffer_pool(size_t s)
        {
            static int i=0;
            if( m_bPool == nullptr)
            {
                std::string name = "mesh_manager_buffer_pool_" + std::to_string(i++);
                m_bPool = get_parent_context()->new_buffer_pool(name);
                m_bPool->set_size(s);
                m_bPool->create();
            }
        }

        /**
         * @brief unload_mesh
         * @param name
         *
         * Removes a mesh from the manager and frees all the buffers
         * associated with it. All references to the mesh are now
         * invalid.
         */
        void unload_mesh(std::string const & name)
        {
            auto f = m_meshs.find(name);
            if( f == m_meshs.end() )
            {
                return;
            }

            for(uint32_t i = 0 ; i < f->second->m_num_attributes; i++)
            {
                if(f->second->m_attributes[i].buffer)
                {
                    m_bPool->free_buffer( f->second->m_attributes[i].buffer );
                    f->second->m_attributes[i].buffer = nullptr;
                    f->second->m_attributes[i].vertex_size = 0;
                }
            }

            f->second->m_index_count    = 0;
            f->second->m_num_attributes = 0;

            m_meshs.erase(name);
        }

        buffer_pool * get_buffer_pool()
        {
            return m_bPool;
        }
        std::shared_ptr<mesh> get_mesh(std::string const & name)
        {
            auto f = m_meshs.find(name);
            if( f == m_meshs.end() )
            {
                throw std::runtime_error( std::string("Mesh with name ") + name + " does not exists.");
            }
            return f->second;
        }

        std::shared_ptr<mesh> new_mesh(std::string const & name)
        {
            if( m_meshs.count(name) == 0)
            {
                std::shared_ptr<mesh> M( new mesh(this) );
                m_meshs[name] = M;

                return M;
            }
            throw std::runtime_error( std::string("Mesh with name ") + name + " already exists.");

        }

    private:
        vka::buffer_pool                            * m_bPool = nullptr;
        std::map<std::string, std::shared_ptr<mesh> > m_meshs;
};


inline bool mesh::allocate()
{
    auto num_vertices = m_vertex_count;
    for(uint32_t i=0;i<m_attributes.size() ;i++)
    {
        if( m_attributes[i].buffer )
        {
            m_manager->get_buffer_pool()->free_buffer( m_attributes[i].buffer );
            m_attributes[i].buffer = nullptr;
        }
    }

    for(uint32_t i=0;i<m_attributes.size() ;i++)
    {
        if( m_attributes[i].vertex_size != 0)
        {
            m_attributes[i].buffer = m_manager->get_buffer_pool()->new_buffer(num_vertices*m_attributes[i].vertex_size , m_attributes[i].vertex_size );
            if( m_attributes[i].buffer == nullptr)
            {
                // clear all buffers
                return false;
            }
        }
    }

    if( m_index_buffer)
    {
        m_manager->get_buffer_pool()->free_buffer( m_index_buffer );
    }
    m_index_buffer  = m_manager->get_buffer_pool()->new_buffer( m_index_count*m_index_size, m_index_size);
    return true;
}



}
