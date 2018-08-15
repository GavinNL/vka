#ifndef VKA_DESCRIPTOR_POOL_H
#define VKA_DESCRIPTOR_POOL_H

#include <vulkan/vulkan.hpp>
#include "context_child.h"
#include <vka/core/descriptor_set.h>
#include <map>
#include <set>

namespace vka
{

class DescriptorPool : public context_child
{
public:

    DescriptorPool(context * parent) : context_child(parent)
    {

    }

    ~DescriptorPool();

    operator vk::DescriptorPool()
    {
        return m_descriptor_pool;
    }


    DescriptorPool* set_pool_size( vk::DescriptorType t, uint32_t s)
    {
        m_pools[t].descriptorCount = s;
        return this;
    }
    void create();

    DescriptorSet_p allocateDescriptorSet();
    //==========================================

private:

    vk::DescriptorPool m_descriptor_pool;

    std::map<vk::DescriptorType, vk::DescriptorPoolSize> m_pools;


    std::set<DescriptorSet_p> m_Sets;

    friend class context;
    friend class deleter<DescriptorPool>;
    friend class DescriptorSet;
};

}

#endif
