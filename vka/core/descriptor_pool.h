#ifndef VKA_DESCRIPTOR_POOL_H
#define VKA_DESCRIPTOR_POOL_H

#include <vulkan/vulkan.hpp>
#include "context_child.h"
#include <map>
#include <set>

namespace vka
{

class context;
class descriptor_set;

class descriptor_pool : public context_child
{
public:
    CONTEXT_CHILD_DEFAULT_CONSTRUCTOR(descriptor_pool)

    ~descriptor_pool();

    operator vk::DescriptorPool()
    {
        return m_descriptor_pool;
    }


    descriptor_pool* set_pool_size( vk::DescriptorType t, uint32_t s)
    {
        m_pools[t].descriptorCount = s;
        return this;
    }
    void create();


    descriptor_set* allocate_descriptor_set();

    //==========================================

private:
    //void remove_set()

    vk::DescriptorPool m_descriptor_pool;

    std::map<vk::DescriptorType, vk::DescriptorPoolSize> m_pools;

    std::set<vka::descriptor_set*> m_sets;
    friend class context;
    friend class deleter<descriptor_pool>;
    friend class descriptor_set;
};

}

#endif
