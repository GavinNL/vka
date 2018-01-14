#include <vka/core/buffer.h>
#include <vka/core/device_memory.h>
#include <vka/core/context.h>
#include <vka/core/array_view.h>


vka::buffer::~buffer()
{
    //if( get_Par )
    {
        if(m_buffer)
        {
            get_device().destroyBuffer(m_buffer);
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

    m_memory.allocate( this );
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
