#include <vka/core/command_pool.h>
#include <vka/core/context.h>

vka::command_pool::~command_pool()
{
    if( m_parent_context)
    {
        m_parent_context->get_device().destroyCommandPool(m_command_pool);
    }
}
