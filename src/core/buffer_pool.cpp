#include <vka/core/context.h>
#include <vka/utils/buffer_pool.h>

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


    auto * S = new vka::sub_buffer();

    S->m_buffer = vk_sub_buff;
    S->m_size   = req.size;
    S->m_offset = offset;

    m_sub_buffers.insert(S);

    return S;
}

void vka::sub_buffer::copy(void * data, vk::DeviceSize size, vk::DeviceSize offset)
{
    m_parent->copy(data, size, m_offset+offset);
}
