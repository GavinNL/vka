#include <vka/core/semaphore.h>
#include <vka/core/context.h>

vka::semaphore::semaphore(vka::context * parent) : m_parent_context(parent)
{
    m_semaphore = m_parent_context->get_device().createSemaphore( vk::SemaphoreCreateInfo() );
    if(!m_semaphore)
    {
        throw std::runtime_error("Error creating semaphore");
    }
}

vka::semaphore::~semaphore()
{
    if(m_semaphore)
        m_parent_context->get_device().destroySemaphore(m_semaphore);
}
