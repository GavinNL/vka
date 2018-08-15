#include <vka/core/descriptor_pool.h>
#include <vka/core/descriptor_set.h>
#include <vka/core/log.h>

vka::DescriptorPool::~DescriptorPool()
{
    if(m_descriptor_pool)
    {
        get_device().destroyDescriptorPool( m_descriptor_pool );

    }
}

void vka::DescriptorPool::create()
{
    if( m_pools.size() ==0 )
    {
        throw std::runtime_error("No descriptor pools set");
    }

    std::vector<vk::DescriptorPoolSize> pools;
    for(auto & p : m_pools)
    {
        pools.push_back(p.second);
        pools.back().type = p.first;
    }

    vk::DescriptorPoolCreateInfo poolInfo;

    poolInfo.poolSizeCount = static_cast<uint32_t>( pools.size() );
    poolInfo.pPoolSizes    = pools.data();
    poolInfo.maxSets       = 10;
#warning WHy is the above 10?

    m_descriptor_pool = get_device().createDescriptorPool(poolInfo);

    if ( !m_descriptor_pool )
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }

    LOG << "Descriptor Pool created" << ENDL;
}

vka::DescriptorSet_p vka::DescriptorPool::allocateDescriptorSet()
{
    auto set = std::make_shared<vka::descriptor_set>(get_parent_context());

    m_Sets.insert(set);
    set->m_parent_pool = this;

    return set;
}
