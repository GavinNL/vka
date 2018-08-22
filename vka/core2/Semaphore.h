#ifndef VKA_SEMAPHORE_H
#define VKA_SEMAPHORE_H

#include <vulkan/vulkan.hpp>
//#include "deleter.h"
#include <vka/core/context_child.h>

namespace vka
{

class context;

class Semaphore : public context_child
{
    public:
    operator vk::Semaphore()
    {
        return m_semaphore;
    }

    vk::Semaphore & get()
    {
        return m_semaphore;
    }

    void destroy()
    {
        if(m_semaphore)
        {
            get_device().destroySemaphore(m_semaphore);
            m_semaphore = vk::Semaphore();
        }

    }

    ~Semaphore()
    {
        destroy();
    }
    private:
    Semaphore(context * parent);

     vk::Semaphore m_semaphore;

     friend class context;
};

using Semaphore_p = std::shared_ptr<vka::Semaphore>;
}

#endif
