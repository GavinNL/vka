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
#include <vka/core2/MeshObject.h>
#include <vka/core2/RenderTarget2.h>
#include <vka/core2/Screen.h>

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

void command_buffer::bindMeshObject(const MeshObject &obj)
{
    if( obj.GetIndexBuffer() )
    {
        bindIndexSubBuffer( obj.GetIndexBuffer(), obj.GetIndexType() );
    }

    for(auto & b : obj.GetAttributeBuffers() )
    {
        bindVertexSubBuffer( b.first, b.second);
    }
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


void command_buffer::convertTextureLayer(std::shared_ptr<vka::Texture> & tex,
                                         uint32_t layer, uint32_t layer_count,
                                         vk::ImageLayout new_layout,
                                         vk::PipelineStageFlags srcStageMask,
                                         vk::PipelineStageFlags dstStageMask)
{
    vk::ImageSubresourceRange R;
    R.baseMipLevel = 0;
    R.levelCount = tex->GetMipLevels();
    R.baseArrayLayer = layer;
    R.layerCount = layer_count;
    R.aspectMask = vk::ImageAspectFlagBits::eColor;
    convertTexture( tex,
                    tex->GetLayout(0,layer), // old layout, all mips must be the same
                    new_layout,
                    R,
                    srcStageMask,
                    dstStageMask
                    );
}

void command_buffer::convertTextureLayerMips(std::shared_ptr<vka::Texture> & tex,
                                             uint32_t layer, uint32_t layer_count,
                                             uint32_t mipLevel, uint32_t mipLevelCount,
                                             vk::ImageLayout old_layout, vk::ImageLayout new_layout,
                                             vk::PipelineStageFlags srcStageMask,
                                             vk::PipelineStageFlags dstStageMask)
{
    vk::ImageSubresourceRange R;
    R.baseMipLevel = mipLevel;
    R.levelCount  = mipLevelCount;
    R.baseArrayLayer = layer;
    R.layerCount = layer_count;
    R.aspectMask = vk::ImageAspectFlagBits::eColor;
    convertTexture( tex,
                    old_layout, // old layout, all mips must be the same
                    new_layout,
                    R,
                    srcStageMask,
                    dstStageMask
                    );
}


void command_buffer::convertTexture( std::shared_ptr<vka::Texture> & tex,
                                     vk::ImageLayout old_layout ,
                                     vk::ImageLayout new_layout ,
                                     vk::ImageSubresourceRange const & range,
                                     vk::PipelineStageFlags srcStageMask,
                                     vk::PipelineStageFlags dstStageMask
                                     )
{

    LOG << "Converting Texture: " << ENDL;
    LOG << "           Array Layer: (" << range.baseArrayLayer << ", " << range.layerCount << ENDL;
    LOG << "           Mip Level  : (" << range.baseMipLevel << ", " << range.levelCount << ENDL;
    LOG << "          From        : (" << vk::to_string(old_layout ) << ENDL;
    LOG << "          To          : (" << vk::to_string(new_layout ) << ENDL;


    vk::ImageMemoryBarrier barrier;

    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;

    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = tex->GetImage();

    barrier.subresourceRange = range;

    auto & imageMemoryBarrier = barrier;
    switch (old_layout)
    {
    case vk::ImageLayout::eUndefined: // VK_IMAGE_LAYOUT_UNDEFINED:
        // Image layout is undefined (or does not matter)
        // Only valid as initial layout
        // No flags required, listed only for completeness
        imageMemoryBarrier.srcAccessMask = vk::AccessFlags();
        break;

    case vk::ImageLayout::ePreinitialized: //:
        // Image is preinitialized
        // Only valid as initial layout for linear images, preserves memory contents
        // Make sure host writes have been finished
        imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;// VK_ACCESS_HOST_WRITE_BIT;
        break;

    case vk::ImageLayout::eColorAttachmentOptimal: //:
        // Image is a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;//VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case vk::ImageLayout::eDepthStencilAttachmentOptimal: //:
        // Image is a depth/stencil attachment
        // Make sure any writes to the depth/stencil buffer have been finished
        imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;//VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case vk::ImageLayout::eTransferSrcOptimal: //:
        // Image is a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;//VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case vk::ImageLayout::eTransferDstOptimal: //:
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;//VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case vk::ImageLayout::eShaderReadOnlyOptimal: //:
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;//VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (new_layout)
    {
    case vk::ImageLayout::eTransferDstOptimal: //:
        // Image will be used as a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;//VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case vk::ImageLayout::eTransferSrcOptimal: //:
        // Image will be used as a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;//VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case vk::ImageLayout::eColorAttachmentOptimal: //:
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;//VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case vk::ImageLayout::eDepthStencilAttachmentOptimal: //:
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | vk::AccessFlagBits::eDepthStencilAttachmentWrite;//VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case vk::ImageLayout::eShaderReadOnlyOptimal: //:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if (imageMemoryBarrier.srcAccessMask == vk::AccessFlags())
        {
            imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite;// ;//VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;//VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    pipelineBarrier( srcStageMask,
                     dstStageMask,
                     vk::DependencyFlags(),0,0,barrier);


    for(uint32_t i=range.baseArrayLayer; i<range.layerCount;i++)
    {
        for(uint32_t j=range.baseMipLevel; j<range.levelCount;j++)
        {
            tex->m_LayoutsA.at(i).at(j) = new_layout;
        }
    }

}



void command_buffer::blitMipMap( std::shared_ptr<vka::Texture> & tex,
                                uint32_t Layer, uint32_t LayerCount,
                                uint32_t src_miplevel,
                                uint32_t dst_miplevel)
{
    auto & extents = tex->GetExtents();
    vk::ImageBlit imgBlit;

    // Source
    imgBlit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imgBlit.srcSubresource.baseArrayLayer = Layer;
    imgBlit.srcSubresource.layerCount = LayerCount;
    imgBlit.srcSubresource.mipLevel   = src_miplevel;


    imgBlit.srcOffsets[1].x = int32_t( extents.width  >> src_miplevel);
    imgBlit.srcOffsets[1].y = int32_t( extents.height >> src_miplevel);
    imgBlit.srcOffsets[1].z = std::max( int32_t(1), int32_t( extents.depth >> src_miplevel));

    // Destination
    imgBlit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imgBlit.dstSubresource.baseArrayLayer = Layer;
    imgBlit.dstSubresource.layerCount = LayerCount;
    imgBlit.dstSubresource.mipLevel   = dst_miplevel;

    imgBlit.dstOffsets[1].x = int32_t( extents.width  >> dst_miplevel);
    imgBlit.dstOffsets[1].y = int32_t( extents.height >> dst_miplevel);
    imgBlit.dstOffsets[1].z = std::max( int32_t(1), int32_t( extents.depth >> dst_miplevel) );



    blitImage( tex->GetImage(), vk::ImageLayout::eTransferSrcOptimal,
               tex->GetImage(), vk::ImageLayout::eTransferDstOptimal,
               imgBlit, vk::Filter::eLinear);
}



void command_buffer::generateMipMaps( std::shared_ptr<vka::Texture> & Tex,
                                      uint32_t Layer, uint32_t LayerCount)
{
    // Convert mip level 0 to SrcOptimal
    convertTextureLayerMips( Tex,
                             Layer,LayerCount, // layers 0-1
                             0,1, // mips level i+1
                             Tex->GetLayout(0,Layer), vk::ImageLayout::eTransferSrcOptimal,
                             vk::PipelineStageFlagBits::eTransfer,
                             vk::PipelineStageFlagBits::eHost);

    for(uint32_t i=1; i < Tex->GetMipLevels() ; i++)
    {
        // Convert Layer i to TransferDst
        convertTextureLayerMips( Tex,
                                 Layer,LayerCount, // layers 0-1
                                 i,1, // mips level i+1
                                 Tex->GetLayout(i,Layer), vk::ImageLayout::eTransferDstOptimal,
                                 vk::PipelineStageFlagBits::eTransfer,
                                 vk::PipelineStageFlagBits::eHost);

        // Blit from miplevel i-1 to i for layers 0 and 1
        blitMipMap( Tex,
                    Layer,LayerCount,
                    i-1,i);


        // convert layer i into src so it can be copied from in the next iteraation
        convertTextureLayerMips( Tex,
                                 Layer,LayerCount, // layers 0-1
                                 i,1, // mips level i+1
                                 vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal,
                                 vk::PipelineStageFlagBits::eHost,
                                 vk::PipelineStageFlagBits::eTransfer);

    }
    convertTextureLayerMips( Tex,
                             Layer,LayerCount, // layers 0-1
                             0,Tex->GetMipLevels(), // mips level i+1
                             vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                             vk::PipelineStageFlagBits::eHost,
                             vk::PipelineStageFlagBits::eTransfer);
}


void command_buffer::beginRender(RenderTarget2 & target)
{
    vk::RenderPassBeginInfo m_renderpass_info;

    m_renderpass_info.renderPass        = target.GetRenderPass();// *get_renderpass();
    m_renderpass_info.framebuffer       = target.GetFramebuffer();// *get_framebuffer();
    m_renderpass_info.clearValueCount   = target.GetClearValues().size();
    m_renderpass_info.pClearValues      = target.GetClearValues().data();//m_clear_values.data();
    m_renderpass_info.renderArea.extent = target.GetExtent();

    beginRenderPass(m_renderpass_info, vk::SubpassContents::eInline);
}

void command_buffer::beginRender(Screen & target, uint32_t frame_buffer_index)
{
    vk::RenderPassBeginInfo m_renderpass_info;

    m_renderpass_info.renderPass        = target.GetRenderPass();// *get_renderpass();
    m_renderpass_info.framebuffer       = target.GetFramebuffer(frame_buffer_index);// *get_framebuffer();
    m_renderpass_info.clearValueCount   = target.GetClearValues().size();
    m_renderpass_info.pClearValues      = target.GetClearValues().data();//m_clear_values.data();
    m_renderpass_info.renderArea.extent = target.GetExtent();

    beginRenderPass(m_renderpass_info, vk::SubpassContents::eInline);
}



}
