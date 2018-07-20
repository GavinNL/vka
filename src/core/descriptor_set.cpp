#include <vka/core/descriptor_set_layout.h>
#include <vka/core/descriptor_set.h>
#include <vka/core/descriptor_pool.h>
#include <vka/core/texture.h>
#include <vka/core/buffer.h>
#include <vka/core/context.h>


#include <vka/utils/buffer_pool.h>

#include <vka/core2/TextureMemoryPool.h>
#include <vka/core2/BufferMemoryPool.h>

#include <vka/core/log.h>

vka::descriptor_set::~descriptor_set()
{

}

void vka::descriptor_set::create(std::vector< vk::DescriptorSetLayoutBinding > const & bindings)
{
    m_bindings = bindings;
    auto * dsl = get_parent_context()->new_descriptor_set_layout(m_bindings);


    vk::DescriptorSetAllocateInfo         info;

    info.setDescriptorPool( *m_parent_pool );
    info.pSetLayouts        = &dsl->get();
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

vka::descriptor_set * vka::descriptor_set::attach_uniform_buffer(uint32_t index,
                     const buffer * buff,
                     vk::DeviceSize size,
                     vk::DeviceSize offset)
{

    DescriptorInfo bufferInfo;
    bufferInfo.type = DescriptorInfo::Buffer;
    bufferInfo.buffer.buffer = *buff;
    bufferInfo.buffer.offset = offset;
    bufferInfo.buffer.range  = size;

    m_DescriptorInfos[index] = bufferInfo;

    return this;
}

vka::descriptor_set * vka::descriptor_set::attach_uniform_buffer(uint32_t index,
                     const sub_buffer * buff,
                     vk::DeviceSize size,
                     vk::DeviceSize offset)
{

    DescriptorInfo bufferInfo;
    bufferInfo.type = DescriptorInfo::Buffer;
    bufferInfo.buffer.buffer = *buff;
    bufferInfo.buffer.offset = offset;
    bufferInfo.buffer.range  = size;

    m_DescriptorInfos[index] = bufferInfo;

    return this;
}

vka::descriptor_set * vka::descriptor_set::attach_dynamic_uniform_buffer(uint32_t index,
                     const buffer * buff,
                     vk::DeviceSize size,
                     vk::DeviceSize offset)
{

    DescriptorInfo bufferInfo;
    bufferInfo.type = DescriptorInfo::DynamicBuffer;
    bufferInfo.buffer.buffer = *buff;
    bufferInfo.buffer.offset = offset;
    bufferInfo.buffer.range  = size;

    m_DescriptorInfos[index] = bufferInfo;

    return this;
}

vka::descriptor_set * vka::descriptor_set::attach_dynamic_uniform_buffer(uint32_t index,
                     const sub_buffer * buff,
                     vk::DeviceSize size,
                     vk::DeviceSize offset)
{

    DescriptorInfo bufferInfo;
    bufferInfo.type = DescriptorInfo::DynamicBuffer;
    bufferInfo.buffer.buffer = *buff;
    bufferInfo.buffer.offset = offset;
    bufferInfo.buffer.range  = size;

    m_DescriptorInfos[index] = bufferInfo;

    return this;
}





//------------------

vka::descriptor_set * vka::descriptor_set::AttachUniformBuffer(uint32_t index,
                     std::shared_ptr<SubBuffer> & sub_buffer ,
                     vk::DeviceSize size,
                     vk::DeviceSize offset)
{

    DescriptorInfo bufferInfo;
    bufferInfo.type = DescriptorInfo::Buffer;
    bufferInfo.buffer.buffer = sub_buffer->GetParentBufferHandle();
    bufferInfo.buffer.offset = sub_buffer->GetOffset() + offset;
    bufferInfo.buffer.range  = size;

    m_DescriptorInfos[index] = bufferInfo;

    return this;
}


vka::descriptor_set * vka::descriptor_set::AttachSampler( uint32_t index,
                                                          std::shared_ptr<vka::Texture> & texture,
                                                          vk::ImageView view,
                                                          vk::Sampler sampler
                                                          )
{
    vka::DescriptorInfo imageInfo;

    imageInfo.type              = DescriptorInfo::Image;
    imageInfo.image.imageLayout = texture->GetLayout();//->get_layout();// texture.get().m_CreateInfo.initialLayout;
    imageInfo.image.imageView   = view;// texture.get().m_View;
    imageInfo.image.sampler     = sampler;// texture.get().m_Sampler;

    m_DescriptorInfos[index] = imageInfo;

    return this;
}

vka::descriptor_set * vka::descriptor_set::AttachSampler(uint32_t index,
                                                         std::shared_ptr<vka::Texture> &texture,
                                                         std::string const & view_name,
                                                         std::string const & sampler_name)
{
    return AttachSampler(index, texture, texture->GetImageView(view_name), texture->GetSampler(sampler_name));
}
