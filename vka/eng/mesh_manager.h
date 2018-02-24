#include "manager_base.h"
#include <vka/core/context.h>
#include <vka/utils/buffer_pool.h>

namespace vka
{

class mesh_manager;

class mesh
{
#define MAX_ATTRIBUTES 10
   public:
    mesh(mesh_manager *m) : m_manager(m)
    {
        for(uint32_t i=0;i<MAX_ATTRIBUTES;i++)
            m_attributes[i] = nullptr;
    }

    ~mesh()
    {

    }

    uint32_t num_attributes() const
    {
        return m_num_attributes;
    }

    /**
     * @brief allocate_attribute
     * @param i
     * @param bytes_size
     * @param attribute_byte_size
     * @return
     *
     * Allocates
     */
    bool allocate_vertex_data(uint32_t num_vertices, std::vector<uint32_t> const & vertex_attribute_size);

   private:

       mesh_manager                                * m_manager = nullptr;
       std::array< vka::sub_buffer*, MAX_ATTRIBUTES> m_attributes;
       uint32_t                                      m_num_attributes=0;

       uint32_t                                      m_index_count = 0;

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
                if(f->second->m_attributes[i])
                {
                    m_bPool->free_buffer( f->second->m_attributes[i] );
                    f->second->m_attributes[i] = nullptr;
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
            if( f != m_meshs.end() )
            {
                throw std::runtime_error( std::string("Mesh with name ") + name + " already exists.");
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


inline bool mesh::allocate_vertex_data(uint32_t num_vertices, std::vector<uint32_t> const & vertex_attribute_size)
{
    assert( vertex_attribute_size.size() < MAX_ATTRIBUTES);
    for(uint32_t i=0;i<vertex_attribute_size.size() ;i++)
    {
        if( m_attributes[i] )
        {
            m_manager->get_buffer_pool()->free_buffer( m_attributes[i] );
            m_attributes[i] = nullptr;
        }
    }

    for(uint32_t i=0;i<vertex_attribute_size.size() ;i++)
    {
        m_attributes[i] = m_manager->get_buffer_pool()->new_buffer(num_vertices*vertex_attribute_size[i] , vertex_attribute_size[i] );
    }
    m_num_attributes = vertex_attribute_size.size();
}

}
