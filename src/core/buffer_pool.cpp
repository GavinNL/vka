#include <vka/core/context.h>
#include <vka/utils/buffer_pool.h>
#include <vka/core/context.h>
#include <vka/core/command_pool.h>

void vka::buffer_pool::free_buffer(sub_buffer * b)
{
    if( m_sub_buffers.find(b) != m_sub_buffers.end() )
    {
        m_sub_buffers.erase(b);
        m_manager.free(b->offset());

       // get_parent_context()->get_device().destroyBuffer(b->m_buffer);

        LOG << "Buffer free: " << b->size() << " bytes" << ENDL;

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
        ERROR << "Could not allocate " << n << " bytes with alignment " << alignment << ENDL;
        return nullptr;
    }


    auto * S = new vka::sub_buffer( this );

    S->m_DescriptorBufferInfo.buffer = m_buffer;
    S->m_DescriptorBufferInfo.range  = n;
    S->m_DescriptorBufferInfo.offset = offset;

    m_sub_buffers.insert(S);

    return S;
}


void vka::sub_buffer::copy(void const * data, vk::DeviceSize size, vk::DeviceSize _offset )
{
    m_parent->copy( data, size, offset() + _offset);
}

