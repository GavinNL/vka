#include <vka/core/context.h>
#include <vka/utils/buffer_pool.h>
#include <vka/core/context.h>
#include <vka/core/command_pool.h>

void vka::buffer_pool::free_buffer(sub_buffer * b)
{
    if( m_sub_buffers.find(b) != m_sub_buffers.end() )
    {
        m_sub_buffers.erase(b);
        m_manager.free(b->m_offset);

       // get_parent_context()->get_device().destroyBuffer(b->m_buffer);

        LOG << "Buffer free: " << b->m_size << " bytes" << ENDL;

        delete b;
    } else {

        throw std::runtime_error("That sub_buffer is not a child of the buffer pool");
    }

}

vka::buffer_pool::~buffer_pool()
{
    for(auto * p : m_sub_buffers)
    {
        free_buffer(p);
    }
    m_sub_buffers.clear();
}

vka::sub_buffer *vka::buffer_pool::new_buffer(vk::DeviceSize n, vk::DeviceSize alignment)
{
    auto offset = m_manager.allocate( n , alignment);

    if(offset == m_manager.error)
    {

        return nullptr;
    }



    auto * S = new vka::sub_buffer( this );

    S->m_buffer   = m_buffer;
    S->m_size     = n;
    S->m_offset   = offset;
    S->clear_buffer_objects();

    m_sub_buffers.insert(S);

    return S;
}



vka::sub_buffer_object vka::sub_buffer::reserve(vk::DeviceSize size, vk::DeviceSize alignment)
{
    sub_buffer_object obj;
    auto offset = m_manager.allocate(size, alignment);
    if( offset != m_manager.error)
    {
        obj.m_offset = offset;
        obj.m_size   = size;
    } else {
        obj.m_offset = m_manager.error;
        obj.m_size   = 0;
    }
    return obj;
}

vka::sub_buffer_object vka::sub_buffer::insert(void const * data, vk::DeviceSize size, vk::DeviceSize alignment)
{
    sub_buffer_object obj;

    auto offset = m_manager.allocate(size, alignment);

    if( offset != m_manager.error)
    {
        m_parent->copy(data, size, offset+m_offset);
        //copy(data, size, offset);

        obj.m_offset = offset;
        obj.m_size   = size;
    }
    else
    {
        obj.m_offset = m_manager.error;
        obj.m_size   = 0;
    }

    return obj;
}

void vka::sub_buffer::free_buffer_boject( const sub_buffer_object & obj)
{
    m_manager.free(obj.m_offset);
}

void vka::sub_buffer::clear_buffer_objects()
{
    m_manager.reset( m_size );
}
