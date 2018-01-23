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

        get_parent_context()->get_device().destroyBuffer(b->m_buffer);

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

vka::sub_buffer *vka::buffer_pool::new_buffer(size_t n)
{
    vk::BufferCreateInfo Info;
    Info.size = n;
    Info.usage = this->m_create_info.usage;

    auto vk_sub_buff = get_parent_context()->get_device().createBuffer(Info);

    if(!vk_sub_buff)
    {
        throw std::runtime_error("Could not create buffer");
    }

    auto req = get_device().getBufferMemoryRequirements(vk_sub_buff);

    n = (n/req.alignment + (( n%req.alignment==0) ? 0 : 1))*req.alignment;

    auto offset = m_manager.allocate( n , req.alignment);

    if(offset == m_manager.error)
    {
        get_device().destroyBuffer(vk_sub_buff);
        return nullptr;
    }

    get_device().bindBufferMemory( vk_sub_buff , m_memory, offset);


    auto * S = new vka::sub_buffer( this );

    S->m_buffer = vk_sub_buff;
    S->m_size   = req.size;
    S->m_offset = offset;
    S->clear_buffer_objects();

    m_sub_buffers.insert(S);

    return S;
}

//void vka::sub_buffer::copy(void const * data, vk::DeviceSize size, vk::DeviceSize offset)
//{
//    m_parent->copy(data, size, m_offset+offset);
//}


void vka::sub_buffer::copy(const void *data, vk::DeviceSize _size, vk::DeviceSize offset)
{

        auto * C =  m_parent->get_parent_context();
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

}


vka::sub_buffer_object vka::sub_buffer::insert(void const * data, vk::DeviceSize size)
{
    sub_buffer_object obj;

    auto offset = m_manager.allocate(size);

    if( offset != m_manager.error)
    {
        copy(data, size, offset);

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
