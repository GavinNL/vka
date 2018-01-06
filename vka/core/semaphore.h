#ifndef VKA_SEMAPHORE_H
#define VKA_SEMAPHORE_H

#include <vulkan/vulkan.hpp>
#include "deleter.h"

namespace vka
{

class context;

class semaphore
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
        context *m_parent_context=nullptr;
        friend class context;
        friend class deleter<semaphore>;
};

}

#endif
