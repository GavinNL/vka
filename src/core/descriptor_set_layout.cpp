#include <vka/core/descriptor_set_layout.h>
#include <vka/core2/DescriptorPool.h>
#include <vka/core/context.h>

#include <vka/core/log.h>

vka::DescriptorLayoutSet::~DescriptorLayoutSet()
{
    clear();
}

void vka::DescriptorLayoutSet::clear()
{
    if(m_descriptor_set_layout)
    {
        LOG << "Destroying descriptor set layout" << ENDL;
        get_device().destroyDescriptorSetLayout(m_descriptor_set_layout);
        m_descriptor_set_layout = vk::DescriptorSetLayout();
    }
}

vka::DescriptorLayoutSet *vka::DescriptorLayoutSet::addTextureLayoutBinding(uint32_t binding, vk::ShaderStageFlags stages)
{
    vk::DescriptorSetLayoutBinding samplerLayoutBinding;
    samplerLayoutBinding.binding            = binding;
    samplerLayoutBinding.descriptorCount    = 1;
    samplerLayoutBinding.descriptorType     = vk::DescriptorType::eCombinedImageSampler;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags         = stages;// VK_SHADER_STAGE_VERTEX_BIT;

    m_DescriptorSetLayoutBindings.push_back(samplerLayoutBinding);

    return this;
}

vka::DescriptorLayoutSet *vka::DescriptorLayoutSet::addUniformLayoutBinding(uint32_t binding, vk::ShaderStageFlags stages)
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding;
    uboLayoutBinding.binding            = binding;
    uboLayoutBinding.descriptorCount    = 1;
    uboLayoutBinding.descriptorType     = vk::DescriptorType::eUniformBuffer;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags         = stages;// VK_SHADER_STAGE_VERTEX_BIT;

    m_DescriptorSetLayoutBindings.push_back(uboLayoutBinding);

    return this;
}

vka::DescriptorLayoutSet *vka::DescriptorLayoutSet::addDynamicUniformLayoutBinding(uint32_t binding, vk::ShaderStageFlags stages)
{
    vk::DescriptorSetLayoutBinding duboLayoutBinding;

    duboLayoutBinding.binding            = binding;
    duboLayoutBinding.descriptorCount    = 1;
    duboLayoutBinding.descriptorType     = vk::DescriptorType::eUniformBufferDynamic;
    duboLayoutBinding.pImmutableSamplers = nullptr;
    duboLayoutBinding.stageFlags         = stages;// VK_SHADER_STAGE_VERTEX_BIT;

    m_DescriptorSetLayoutBindings.push_back(duboLayoutBinding);

    return this;
}


void vka::DescriptorLayoutSet::create()
{
    vk::DescriptorSetLayoutCreateInfo   C;

    C.bindingCount = static_cast<uint32_t>( m_DescriptorSetLayoutBindings.size() ) ;
    C.pBindings    = m_DescriptorSetLayoutBindings.data();
    C.flags        = m_Flags;


    m_descriptor_set_layout = get_device().createDescriptorSetLayout(C);
    LOG << "Descriptor layout not created yet. Creating a new one" << ENDL;
    if(!m_descriptor_set_layout)
    {
        throw std::runtime_error("Failed to create Descriptor Set layout");
    }

}

