#include <vka/core/command_buffer.h>
#include <vka/utils/buffer_pool.h>
#include <vka/core/pipeline.h>
#include <vka/core/descriptor_set.h>

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
       bindDescriptorSets( vk::PipelineBindPoint::eGraphics,
                           pipeline->get_layout(),
                           firstSet,
                           vk::ArrayProxy<const vk::DescriptorSet>( set->get()),
                           vk::ArrayProxy<const uint32_t>(dynamic_offset) );

}
