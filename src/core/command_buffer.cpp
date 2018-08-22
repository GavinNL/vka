#include <vka/core/command_buffer.h>
#include <vka/core2/DescriptorSet.h>

#include <vka/core/extensions.h>

#include <vka/core2/TextureMemoryPool.h>
#include <vka/core2/MeshObject.h>
#include <vka/core2/RenderTarget.h>
#include <vka/core2/Screen.h>
#include <vka/core2/Pipeline.h>


#if defined OLD_PIPELINE
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

void vka::command_buffer::pushDescriptorSet( vk::PipelineBindPoint bind_point, vka::pipeline * pipeline, uint32_t set, vka::PushDescriptorInfo const & Info)
{
    pushDescriptorSetKHR(bind_point,
                         pipeline->get_layout(),
                         set,
                         Info.m_writes, vka::ExtDispatcher);
}
#endif





//===================
void vka::CommandBuffer::bindDescriptorSet( vk::PipelineBindPoint pipelineBindPoint,
                        vka::Pipeline const & pipeline,
                        uint32_t firstSet,
                        vka::DescriptorSet_p const & set) const
{
       bindDescriptorSets( pipelineBindPoint,
                           pipeline.getLayout(),
                           firstSet,
                           vk::ArrayProxy<const vk::DescriptorSet>( set->get()),
                           nullptr );

}

void vka::CommandBuffer::bindDescriptorSet( vk::PipelineBindPoint pipelineBindPoint,
                        vka::Pipeline const & pipeline,
                        uint32_t firstSet,
                        vka::DescriptorSet_p const & set,
                        uint32_t dynamic_offset) const
{
       bindDescriptorSets( pipelineBindPoint,
                           pipeline.getLayout(),
                           firstSet,
                           vk::ArrayProxy<const vk::DescriptorSet>( set->get()),
                           vk::ArrayProxy<const uint32_t>(dynamic_offset) );

}

void vka::CommandBuffer::pushDescriptorSet( vk::PipelineBindPoint bind_point, vka::Pipeline const & pipeline, uint32_t set, vka::PushDescriptorInfo const & Info)
{
    pushDescriptorSetKHR(bind_point,
                         pipeline.getLayout(),
                         set,
                         Info.m_writes, vka::ExtDispatcher);
}



//-----------------------------
#include <vka/core2/BufferMemoryPool.h>

namespace vka
{
void CommandBuffer::copySubBuffer( std::shared_ptr<vka::SubBuffer> & src,
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


void CommandBuffer::bindVertexSubBuffer(uint32_t firstBinding,
                                         const std::shared_ptr<SubBuffer> & buffer,
                                         vk::DeviceSize offset) const
{
    vk::CommandBuffer::bindVertexBuffers(firstBinding, buffer->GetParentBufferHandle(),offset+buffer->GetOffset());
}

void CommandBuffer::bindIndexSubBuffer( const std::shared_ptr<SubBuffer> & buffer,
                                         vk::IndexType indexType,
                                         vk::DeviceSize offset) const
{
    vk::CommandBuffer::bindIndexBuffer( buffer->GetParentBufferHandle(), buffer->GetOffset()+offset, indexType);
}

void CommandBuffer::bindMeshObject(const MeshObject &obj)
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

void CommandBuffer::drawMeshObject(const MeshObject & obj , uint32_t instanceCount ,uint32_t firstInstance)
{
    drawIndexed( obj.GetIndexCount(), instanceCount, 0, 0 ,firstInstance);
}


void CommandBuffer::copySubBufferToTexture( const std::shared_ptr<SubBuffer> & buffer,
                                           std::shared_ptr<vka::Texture> & tex,
                                           vk::ImageLayout imageLayout,
                                           vk::BufferImageCopy const & C) const
{
    vk::BufferImageCopy lC = C;

    lC.setBufferOffset( buffer->GetOffset() + C.bufferOffset );
    //lC.setImageOffset(  tex->GetOffset()    + C.imageOffset);

    vk::CommandBuffer::copyBufferToImage(
                buffer->GetParentBufferHandle(),
                tex->getImage(),
                imageLayout,
                lC
                );
}


void CommandBuffer::convertTextureLayer(std::shared_ptr<vka::Texture> & tex,
                                         uint32_t layer, uint32_t layer_count,
                                         vk::ImageLayout new_layout,
                                         vk::PipelineStageFlags srcStageMask,
                                         vk::PipelineStageFlags dstStageMask)
{
    vk::ImageSubresourceRange R;
    R.baseMipLevel = 0;
    R.levelCount = tex->getMipLevels();
    R.baseArrayLayer = layer;
    R.layerCount = layer_count;
    R.aspectMask = vk::ImageAspectFlagBits::eColor;
    convertTexture( tex,
                    tex->getLayout(0,layer), // old layout, all mips must be the same
                    new_layout,
                    R,
                    srcStageMask,
                    dstStageMask
                    );
}

void CommandBuffer::convertTextureLayerMips(std::shared_ptr<vka::Texture> & tex,
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


void CommandBuffer::convertTexture( std::shared_ptr<vka::Texture> & tex,
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
    barrier.image                           = tex->getImage();

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



void CommandBuffer::blitMipMap( std::shared_ptr<vka::Texture> & tex,
                                uint32_t Layer, uint32_t LayerCount,
                                uint32_t src_miplevel,
                                uint32_t dst_miplevel)
{
    auto & extents = tex->getExtents();
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



    blitImage( tex->getImage(), vk::ImageLayout::eTransferSrcOptimal,
               tex->getImage(), vk::ImageLayout::eTransferDstOptimal,
               imgBlit, vk::Filter::eLinear);
}



void CommandBuffer::generateMipMaps( std::shared_ptr<vka::Texture> & Tex,
                                      uint32_t Layer, uint32_t LayerCount)
{
    // Convert mip level 0 to SrcOptimal
    convertTextureLayerMips( Tex,
                             Layer,LayerCount, // layers 0-1
                             0,1, // mips level i+1
                             Tex->getLayout(0,Layer), vk::ImageLayout::eTransferSrcOptimal,
                             vk::PipelineStageFlagBits::eTransfer,
                             vk::PipelineStageFlagBits::eHost);

    for(uint32_t i=1; i < Tex->getMipLevels() ; i++)
    {
        // Convert Layer i to TransferDst
        convertTextureLayerMips( Tex,
                                 Layer,LayerCount, // layers 0-1
                                 i,1, // mips level i+1
                                 Tex->getLayout(i,Layer), vk::ImageLayout::eTransferDstOptimal,
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
                             0,Tex->getMipLevels(), // mips level i+1
                             vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                             vk::PipelineStageFlagBits::eHost,
                             vk::PipelineStageFlagBits::eTransfer);
}


void CommandBuffer::beginRender(RenderTarget & target)
{
    vk::RenderPassBeginInfo m_renderpass_info;

    m_renderpass_info.renderPass        = target.GetRenderPass();// *get_renderpass();
    m_renderpass_info.framebuffer       = target.GetFramebuffer();// *get_framebuffer();
    m_renderpass_info.clearValueCount   = target.GetClearValues().size();
    m_renderpass_info.pClearValues      = target.GetClearValues().data();//m_clear_values.data();
    m_renderpass_info.renderArea.extent = target.GetExtent();

    beginRenderPass(m_renderpass_info, vk::SubpassContents::eInline);
}

void CommandBuffer::beginRender(Screen & target, uint32_t frame_buffer_index)
{
    vk::RenderPassBeginInfo m_renderpass_info;

    m_renderpass_info.renderPass        = target.getRenderPass();// *get_renderpass();
    m_renderpass_info.framebuffer       = target.getFramebuffer(frame_buffer_index);// *get_framebuffer();
    m_renderpass_info.clearValueCount   = target.getClearValues().size();
    m_renderpass_info.pClearValues      = target.getClearValues().data();//m_clear_values.data();
    m_renderpass_info.renderArea.extent = target.getExtent();

    beginRenderPass(m_renderpass_info, vk::SubpassContents::eInline);
}


vka::PushDescriptorInfo &PushDescriptorInfo::attach(uint32_t binding, uint32_t count, std::shared_ptr<SubBuffer> & subBuffer)
{
    vk::WriteDescriptorSet W;

    W.dstSet = vk::DescriptorSet();
    W.dstBinding = binding;
    W.descriptorCount = count;
    W.descriptorType = vk::DescriptorType::eUniformBuffer;//VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;


    auto bi = std::make_shared<vk::DescriptorBufferInfo>();
    bi->buffer = subBuffer->GetParentBufferHandle();
    bi->offset = subBuffer->GetOffset();
    bi->range  = subBuffer->GetSize();

    m_BufferInfo.push_back(bi);

    W.pBufferInfo = bi.get();

    m_writes.push_back(W);
    return *this;
}

vka::PushDescriptorInfo &PushDescriptorInfo::attach(uint32_t binding, uint32_t count, Texture_p & texture)
{
    vk::WriteDescriptorSet W;

    W.dstSet = vk::DescriptorSet();
    W.dstBinding = binding;
    W.descriptorCount = count;
    W.descriptorType = vk::DescriptorType::eCombinedImageSampler;//VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;


    auto bi = std::make_shared<vk::DescriptorImageInfo>();
    bi->sampler = texture->getSampler();
    bi->imageView = texture->getImageView();
    bi->imageLayout =  texture->getLayout();

    m_TextureInfo.push_back(bi);

    W.pImageInfo = bi.get();

    m_writes.push_back(W);
    return *this;
}




}
