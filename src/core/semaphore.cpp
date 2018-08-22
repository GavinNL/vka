#include <vka/core/semaphore.h>
#include <vka/core/context.h>

vka::Semaphore::Semaphore(vka::context * parent) : context_child(parent)
{
    m_semaphore = get_device().createSemaphore( vk::SemaphoreCreateInfo() );
    if(!m_semaphore)
    {
        throw std::runtime_error("Error creating semaphore");
    }
}
