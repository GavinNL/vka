#include <vka/utils/buffer_pool.h>
#include <vka/core/buffer.h>
#include <vka/core/command_buffer.h>
#include <vka/core/pipeline.h>
#include <vka/core/descriptor_set.h>

#include <vka/core/texture.h>
#include <vka/core/texture2darray.h>
#include <vka/core/buffer.h>

#include <vka/core/extensions.h>


#include <vka/core2/TextureMemoryPool.h>

void vka::command_buffer::bindVertexSubBuffer(uint32_t firstBinding,
                              vka::sub_buffer const * buffer , vk::DeviceSize offset) const
{

    bindVertexBuffers(firstBinding, buffer->get(), {buffer->offset()+offset} );

}

void vka::command_buffer::bindIndexSubBuffer(  vka::sub_buffer const * buffer ,
                                                vk::IndexType index_type, vk::DeviceSize offset) const
{

    bindIndexBuffer( buffer->get(), buffer->offset()+offset, index_type);

}

void vka::command_buffer::copySubBuffer( vk::Buffer srcBuffer, sub_buffer const * dstBuffer, const vk::BufferCopy & region ) const
{
    const vk::BufferCopy C{ region.srcOffset, region.dstOffset+dstBuffer->offset(), region.size};
    copyBuffer( srcBuffer , *dstBuffer , C );
}

void vka::command_buffer::copySubBuffer( sub_buffer const * srcBuffer, sub_buffer const * dstBuffer, const vk::BufferCopy & region ) const
{
    const vk::BufferCopy C{ region.srcOffset + srcBuffer->offset(), region.dstOffset+dstBuffer->offset(), region.size};
    copyBuffer( *srcBuffer , *dstBuffer , C );
}

void vka::command_buffer::bindDescriptorSet( vk::PipelineBindPoint pipelineBindPoint,
                        vka::pipeline const * pipeline,
                        uint32_t firstSet,
                        vka::descriptor_set const * set ) const
{
       bindDescriptorSets( pipelineBindPoint,
                           pipeline->get_layout(),
                           firstSet,
                           vk::ArrayProxy<const vk::DescriptorSet>( set->get()),
                           nullptr );
}

void vka::command_buffer::bindDescriptorSet( vk::PipelineBindPoint pipelineBindPoint,
                        vka::pipeline const * pipeline,
                        uint32_t firstSet,
                        vka::descriptor_set const * set,
                        uint32_t dynamic_offset) const
{
       bindDescriptorSets( pipelineBindPoint,
                           pipeline->get_layout(),
                           firstSet,
                           vk::ArrayProxy<const vk::DescriptorSet>( set->get()),
                           vk::ArrayProxy<const uint32_t>(dynamic_offset) );

}




//===================

void vka::command_buffer::pushDescriptorSet( vk::PipelineBindPoint bind_point, vka::pipeline * pipeline, uint32_t set, vka::PushDescriptorInfo const & Info)
{
    pushDescriptorSetKHR(bind_point,
                         pipeline->get_layout(),
                         set,
                         Info.m_writes, vka::ExtDispatcher);
}


vka::PushDescriptorInfo &vka::PushDescriptorInfo::attach(uint32_t binding, uint32_t count, vka::texture *texArray)
{
    vk::WriteDescriptorSet W;
    // Descriptor set 0 binding 0  is the texture array
    W.dstSet = vk::DescriptorSet();
    W.dstBinding = binding;
    W.descriptorCount = count;
    W.descriptorType = vk::DescriptorType::eCombinedImageSampler;//VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    W.pImageInfo = &texArray->get_descriptor_info();

    m_writes.push_back(W);
    return *this;
}

vka::PushDescriptorInfo &vka::PushDescriptorInfo::attach(uint32_t binding, uint32_t count, vka::sub_buffer *buffer)
{
    vk::WriteDescriptorSet W;

    W.dstSet = vk::DescriptorSet();
    W.dstBinding = binding;
    W.descriptorCount = count;
    W.descriptorType = vk::DescriptorType::eUniformBuffer;//VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    W.pBufferInfo = &buffer->get_descriptor_info();

    m_writes.push_back(W);
    return *this;
}



//-----------------------------
#include <vka/core2/BufferMemoryPool.h>

namespace vka
{
void command_buffer::copySubBuffer( std::shared_ptr<vka::SubBuffer> & src,
                    std::shared_ptr<vka::SubBuffer> & dst,
                    vk::BufferCopy const & region)
{
    vk::BufferCopy R;
    R.srcOffset = src->GetOffset() + region.srcOffset;
    R.dstOffset = dst->GetOffset() + region.dstOffset;
    R.size      = region.size;

    copyBuffer( src->GetParentBufferHandle(),
                dst->GetParentBufferHandle(),
                R);
}


void command_buffer::bindVertexSubBuffer(uint32_t firstBinding,
                                         const std::shared_ptr<SubBuffer> & buffer,
                                         vk::DeviceSize offset) const
{
    vk::CommandBuffer::bindVertexBuffers(firstBinding, buffer->GetParentBufferHandle(),offset+buffer->GetOffset());
}

void command_buffer::bindIndexSubBuffer( const std::shared_ptr<SubBuffer> & buffer,
                                         vk::IndexType indexType,
                                         vk::DeviceSize offset) const
{
    vk::CommandBuffer::bindIndexBuffer( buffer->GetParentBufferHandle(), buffer->GetOffset()+offset, indexType);
}

void command_buffer::copySubBufferToImage( const std::shared_ptr<SubBuffer> & buffer,
                                           vka::texture * tex,
                                           vk::ImageLayout imageLayout,
                                           vk::BufferImageCopy const & C) const
{
    vk::BufferImageCopy lC = C;
    lC.setBufferOffset( buffer->GetOffset() + C.bufferOffset );
    vk::CommandBuffer::copyBufferToImage(
                buffer->GetParentBufferHandle(),
                tex->get_image(),
                imageLayout,
                lC
                );
}

void command_buffer::copySubBufferToTexture( const std::shared_ptr<SubBuffer> & buffer,
                                           std::shared_ptr<vka::Texture> & tex,
                                           vk::ImageLayout imageLayout,
                                           vk::BufferImageCopy const & C) const
{
    vk::BufferImageCopy lC = C;

    lC.setBufferOffset( buffer->GetOffset() + C.bufferOffset );
    //lC.setImageOffset(  tex->GetOffset()    + C.imageOffset);

    vk::CommandBuffer::copyBufferToImage(
                buffer->GetParentBufferHandle(),
                tex->GetImage(),
                imageLayout,
                lC
                );
}

}
