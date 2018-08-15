#ifndef VKA_SEMAPHORE_H
#define VKA_SEMAPHORE_H

#include <vulkan/vulkan.hpp>
#include "deleter.h"
#include <vka/core/context_child.h>

namespace vka
{

class context;

class semaphore : public context_child
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

    private:
        semaphore(context * parent);
        ~semaphore();

        vk::Semaphore m_semaphore;

        friend class context;
        friend class deleter<semaphore>;
};

}

#endif
