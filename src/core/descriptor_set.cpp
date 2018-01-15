#include <vka/core/descriptor_set.h>
#include <vka/core/descriptor_pool.h>
#include <vka/core/texture.h>
#include <vka/core/context.h>

#include <vka/core/log.h>

vka::descriptor_set_layout::~descriptor_set_layout()
{
    if(m_descriptor_set_layout)
    {
        LOG << "Destroying descriptor set layout" << ENDL;
        get_device().destroyDescriptorSetLayout(m_descriptor_set_layout);
    }
}

vka::descriptor_set_layout *vka::descriptor_set_layout::add_texture_layout_binding(uint32_t binding, vk::ShaderStageFlags stages)
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

vka::descriptor_set_layout *vka::descriptor_set_layout::add_uniform_layout_binding(uint32_t binding, vk::ShaderStageFlags stages)
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

vka::descriptor_set_layout *vka::descriptor_set_layout::add_dynamic_uniform_layout_binding(uint32_t binding, vk::ShaderStageFlags stages)
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


void vka::descriptor_set_layout::create()
{
    //ScreenData_t & ScreenData = vka::Screen::GetGlobal();
    //auto & D = ScreenData.m_DescriptorSetLayouts;

    vk::DescriptorSetLayoutCreateInfo   C;

    C.bindingCount = static_cast<uint32_t>( m_DescriptorSetLayoutBindings.size() ) ;
    C.pBindings    = m_DescriptorSetLayoutBindings.data();



    //auto it = D.find( get().m_DescriptorSetLayoutBindings );
    //
    //if( it == D.end() )
    //{
    m_descriptor_set_layout = get_device().createDescriptorSetLayout(C);
    LOG << "Descriptor layout not created yet. Creating a new one" << ENDL;
    if(!m_descriptor_set_layout)
    {
        throw std::runtime_error("Failed to create Descriptor Set layout");
    }
    //D[ get().m_DescriptorSetLayoutBindings ] = data();

    //}
    //else
    //{
    //    reset( it->second );
    //    LOG << "Descriptor layout already exists. Using that one" << ENDL;
    //}
}

vka::descriptor_set::~descriptor_set()
{

}

void vka::descriptor_set::create(std::vector< vk::DescriptorSetLayoutBinding > const & bindings)
{
    m_bindings = bindings;
    auto * dsl = get_parent_context()->new_descriptor_set_layout(m_bindings);

    vk::DescriptorSetAllocateInfo         info;

    vk::DescriptorSetLayout S = *dsl;

    info.setDescriptorPool( *m_parent_pool );
    info.pSetLayouts        = &S;
    info.descriptorSetCount = 1;

    auto ds = get_device().allocateDescriptorSets( info );
    if( ds.size() == 0)
    {
        throw std::runtime_error("Descriptor set not created");
    }
    m_descriptor_set = ds[0];
}



void vka::descriptor_set::update()
{
    std::vector<vk::WriteDescriptorSet> descriptorWrite;

    for(auto & e : m_DescriptorInfos)
    {
        vk::WriteDescriptorSet w;
        w.dstSet          = m_descriptor_set;
        w.dstBinding      = e.first;
        w.dstArrayElement = 0; // what is this?
        w.descriptorCount = 1;

        switch(e.second.type)
        {
        case DescriptorInfo::DynamicBuffer:
            w.descriptorType  = vk::DescriptorType::eUniformBufferDynamic;
            w.pBufferInfo     = &e.second.buffer;
            break;
        case DescriptorInfo::Buffer:
            w.descriptorType  = vk::DescriptorType::eUniformBuffer;
            w.pBufferInfo     = &e.second.buffer;
            break;
        case DescriptorInfo::Image:
            w.descriptorType  = vk::DescriptorType::eCombinedImageSampler;
            w.pImageInfo      = &e.second.image;
            break;
        default:
            continue;
        }

        descriptorWrite.push_back(w);
    }

    get_device().updateDescriptorSets( descriptorWrite, nullptr);

}


vka::descriptor_set * vka::descriptor_set::attach_sampler( uint32_t index,  vka::texture * texture)
{
    vka::DescriptorInfo imageInfo;
    imageInfo.type              = DescriptorInfo::Image;
    imageInfo.image.imageLayout = texture->get_layout();// texture.get().m_CreateInfo.initialLayout;
    imageInfo.image.imageView   = texture->get_image_view();// texture.get().m_View;
    imageInfo.image.sampler     = texture->get_sampler();// texture.get().m_Sampler;

    m_DescriptorInfos[index] = imageInfo;

    return this;
}
