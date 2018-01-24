#include <vka/core/buffer.h>
#include <vka/core/device_memory.h>
#include <vka/core/context.h>
#include <vka/core/array_view.h>
#include <vka/core/command_pool.h>

vka::buffer::~buffer()
{
    //if( get_Par )
    {
        if(m_buffer)
        {
            get_device().destroyBuffer(m_buffer);
            m_buffer = vk::Buffer();
        }
        LOG << "Buffer destroyed " << ENDL;
    }
}

bool vka::buffer::create()
{
    auto device = get_device();

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

    m_memory.allocate( device.getBufferMemoryRequirements(m_buffer) );
    m_memory.bind(this);

    return true;
}



void * vka::buffer::map_memory()
{
    return m_memory.map();

}

void   vka::buffer::unmap_memory()
{
    m_memory.unmap();
}

bool vka::buffer::copy(const void *data, size_t _size, size_t offset)
{
    if (m_memory.get_memory_properties()  & vk::MemoryPropertyFlagBits::eHostVisible )
    {
        if( offset+_size > size() )
        {
            return false;
        }
        bool already_mapped = m_memory.is_mapped();

        void * m = m_memory.map();
        m = static_cast<char*>(m)+offset;
        memcpy(m,data,_size);

        if(!already_mapped)
            m_memory.unmap();

        return true;
    }
    else
    {
        auto * C = get_parent_context();
        auto * cp = C->get_command_pool();
        auto * sb = C->get_staging_buffer();
        // copy the data to the staging buffer
        if( !sb->copy( data, _size, 0) )
        {
            throw std::runtime_error("Staging buffer is too small");
        }

        vk::CommandBuffer copy_cmd = cp->AllocateCommandBuffer();
        copy_cmd.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );

        copy_cmd.copyBuffer( *sb , *this, vk::BufferCopy( 0, offset , _size ) );
        copy_cmd.end();
        C->submit_cmd_buffer(copy_cmd);
        cp->FreeCommandBuffer(copy_cmd);
        return true;
    }
    return false;
}
