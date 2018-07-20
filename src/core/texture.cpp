#include <vka/core/context.h>
#include <vka/core/texture.h>
#include <vka/core/texture2d.h>
#include <vka/core/command_pool.h>
#include <vka/core/buffer.h>
#include <vka/core/image.h>

vka::texture::texture(vka::context *parent) : context_child(parent), m_Memory(parent)
{
    m_CreateInfo.imageType     = vk::ImageType::e2D;// VK_IMAGE_TYPE_2D;
    //m_CreateInfo.extent.width  = 512;
    //m_CreateInfo.extent.height = height;
    m_CreateInfo.extent.depth  = 1;
    m_CreateInfo.mipLevels     = 1;
    m_CreateInfo.arrayLayers   = 1;
    m_CreateInfo.format        = vk::Format::eR8G8B8A8Unorm;
    m_CreateInfo.tiling        = vk::ImageTiling::eLinear;;

    m_CreateInfo.initialLayout = vk::ImageLayout::ePreinitialized;// VK_IMAGE_LAYOUT_PREINITIALIZED;
    //m_CreateInfo.initialLayout = vk::ImageLayout::eUndefined;// VK_IMAGE_LAYOUT_PREINITIALIZED;
    //m_CreateInfo.initialLayout = vk::ImageLayout::eGeneral;// VK_IMAGE_LAYOUT_PREINITIALIZED;

    m_CreateInfo.usage         = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;// VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    m_CreateInfo.samples       = vk::SampleCountFlagBits::e1; // VK_SAMPLE_COUNT_1_BIT;
    m_CreateInfo.sharingMode   = vk::SharingMode::eExclusive; // VK_SHARING_MODE_EXCLUSIVE;

    m_Memory.set_memory_properties(vk::MemoryPropertyFlagBits::eDeviceLocal);



    m_SamplerInfo.magFilter        = vk::Filter::eLinear;// VK_FILTER_LINEAR;
    m_SamplerInfo.minFilter        = vk::Filter::eLinear;// VK_FILTER_LINEAR;
    m_SamplerInfo.addressModeU     = vk::SamplerAddressMode::eRepeat;//VK_SAMPLER_ADDRESS_MODE_REPEAT;
    m_SamplerInfo.addressModeV     = vk::SamplerAddressMode::eRepeat;//VK_SAMPLER_ADDRESS_MODE_REPEAT;
    m_SamplerInfo.addressModeW     = vk::SamplerAddressMode::eRepeat;//VK_SAMPLER_ADDRESS_MODE_REPEAT;
    m_SamplerInfo.anisotropyEnable = VK_TRUE;
    m_SamplerInfo.maxAnisotropy    = 1;
    m_SamplerInfo.borderColor      = vk::BorderColor::eIntOpaqueBlack;// VK_BORDER_COLOR_INT_OPAQUE_BLACK ;
    m_SamplerInfo.unnormalizedCoordinates = VK_FALSE;
    m_SamplerInfo.compareEnable    = VK_FALSE;
    m_SamplerInfo.compareOp        = vk::CompareOp::eAlways;// VK_COMPARE_OP_ALWAYS;
    m_SamplerInfo.mipmapMode       = vk::SamplerMipmapMode::eLinear;// VK_SAMPLER_MIPMAP_MODE_LINEAR;
    m_SamplerInfo.mipLodBias       = 0.0f;
    m_SamplerInfo.minLod           = 0.0f;
    m_SamplerInfo.maxLod           = 8.0f;

}

vka::texture::~texture()
{
    if( m_Image)
        get_device().destroyImage(m_Image);

    if( m_View)
        get_device().destroyImageView(m_View);

    if(m_Sampler)
        get_device().destroySampler(m_Sampler);
    //if( m_Memory)
    //{
    //    get_device().freeMemory(m_Memory);
    //}
}


uint32_t vka::texture::get_layers() const
{
    return m_CreateInfo.arrayLayers;
}
void vka::texture::set_layers(uint32_t l)
{
    m_CreateInfo.arrayLayers = l;
}

vk::Extent3D vka::texture::get_extents() const
{
    return m_CreateInfo.extent;
}

vka::texture* vka::texture::set_size(vk::DeviceSize w, vk::DeviceSize h, vk::DeviceSize d)
{
    m_CreateInfo.extent.width  = w;
    m_CreateInfo.extent.height = h;
    m_CreateInfo.extent.depth  = d;

    if( d == 1)
    {
        m_CreateInfo.imageType = vk::ImageType::e2D;
    }
    else if( d > 1)
    {
        m_CreateInfo.imageType     = vk::ImageType::e3D;
    }
    return this;
}

vka::texture* vka::texture::set_format(vk::Format F)
{
    m_CreateInfo.format = F;

    set_mipmap_levels( get_mipmap_levels() );
    //m_CreateInfo.mipLevels
    return this;
}

vk::Format vka::texture::get_format() const
{
    return m_CreateInfo.format;
}

vka::texture* vka::texture::set_tiling(vk::ImageTiling T)
{
    m_CreateInfo.tiling = T;
    return this;
}

vk::ImageTiling vka::texture::get_tiling() const
{
    return m_CreateInfo.tiling;
}

vka::texture* vka::texture::set_view_type( vk::ImageViewType type)
{
    m_ViewInfo.viewType = type;
    return this;
}

vk::ImageViewType vka::texture::get_view_type( )
{
    return m_ViewInfo.viewType;
}

void vka::texture::copy_buffer(vk::CommandBuffer cb,
                               vka::buffer *b,
                               vk::BufferImageCopy region)
{
    cb.copyBufferToImage( b->get(), m_Image, vk::ImageLayout::eTransferDstOptimal, region);
}



void vka::texture::copy_image(vka::host_image & D, uint32_t layer, vk::Offset2D E)
{
    auto * b = get_parent_context()->get_staging_buffer();

    void * image_buffer_data = b->map_memory();
    memcpy( image_buffer_data, D.data(), D.size() );
    b->unmap_memory();

    auto cb = get_parent_context()->get_command_pool()->AllocateCommandBuffer();

    cb.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );

        // a. convert the texture to eTransferDstOptimal
        convert_layer(cb, vk::ImageLayout::eTransferDstOptimal, layer,0);

        // b. copy the data from the buffer to the texture
        vk::BufferImageCopy BIC;
        BIC.setBufferImageHeight(  D.height() )
           .setBufferOffset(0) // the image data starts at the start of the buffer
           .setImageExtent( vk::Extent3D( D.width() , D.height(), 1) ) // size of the image
           .setImageOffset( vk::Offset3D( E.x,E.y,0)) // where in the texture we want to paste the image
           .imageSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
                            .setBaseArrayLayer(layer) // the layer to copy
                            .setLayerCount(1) // only copy 2 layers
                            .setMipLevel(0);  // only the first mip-map level

        copy_buffer( cb, b, BIC);

        generate_mipmaps(cb , layer);
    cb.end();
    get_parent_context()->submit_cmd_buffer(cb);
    get_parent_context()->get_command_pool()->FreeCommandBuffer(cb);
}


void vka::texture::set_mipmap_levels( uint32_t levels)
{
#warning The format should not be hard coded
    auto formatProperties = get_physical_device().getFormatProperties(vk::Format::eR8G8B8A8Unorm);

    if( !(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitSrc )  ||
        !(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst ))
    {
        throw std::runtime_error("Bliting of the chosen format is not suported on this device. Set the mipmap levels to zero or choose a different format");
    }

    m_CreateInfo.mipLevels = levels;
}

void vka::texture::generate_mipmaps(vk::CommandBuffer cmdBuff ,
                                    uint32_t layer)
{
    auto L = get_mipmap_levels();

    for(uint32_t m = 0; m < L-1; m++)
    {
        convert_layer(cmdBuff, vk::ImageLayout::eTransferSrcOptimal, layer, m);
        convert_layer(cmdBuff, vk::ImageLayout::eTransferDstOptimal, layer, m+1);

        blit_mipmap(cmdBuff, layer, layer, m, m+1);

        convert_layer(cmdBuff, vk::ImageLayout::eShaderReadOnlyOptimal, layer, m);
    }
}


void vka::texture::blit_mipmap( vk::CommandBuffer & cmdBuff,
                                uint32_t srcLayer,
                                uint32_t dstLayer,
                                uint32_t src_miplevel,
                                uint32_t dst_miplevel)
{
    vk::ImageBlit imgBlit;

    // Source
    imgBlit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imgBlit.srcSubresource.layerCount = 1;
    imgBlit.srcSubresource.baseArrayLayer = srcLayer;
    imgBlit.srcSubresource.mipLevel   = src_miplevel;


    imgBlit.srcOffsets[1].x = int32_t( get_extents().width  >> src_miplevel);
    imgBlit.srcOffsets[1].y = int32_t( get_extents().height >> src_miplevel);
    imgBlit.srcOffsets[1].z = std::max( int32_t(1), int32_t( get_extents().depth>>src_miplevel));

    // Destination
    imgBlit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imgBlit.dstSubresource.layerCount = 1;
    imgBlit.dstSubresource.baseArrayLayer = dstLayer;
    imgBlit.dstSubresource.mipLevel   = dst_miplevel;

    imgBlit.dstOffsets[1].x = int32_t( get_extents().width  >> dst_miplevel);
    imgBlit.dstOffsets[1].y = int32_t( get_extents().height >> dst_miplevel);
    imgBlit.dstOffsets[1].z = std::max( int32_t(1), int32_t( get_extents().depth>>dst_miplevel) );





    LOG << "Generating Mipmap Level: " << dst_miplevel << ENDL;
    cmdBuff.blitImage( get_image(), vk::ImageLayout::eTransferSrcOptimal,
                       get_image(), vk::ImageLayout::eTransferDstOptimal,
                       imgBlit, vk::Filter::eLinear);
}

uint32_t vka::texture::get_mipmap_levels( ) const
{
    return m_CreateInfo.mipLevels;
}

vka::texture* vka::texture::set_memory_properties(vk::MemoryPropertyFlags flags)
{
    m_Memory.set_memory_properties(flags);
    return this;
}

vk::ImageUsageFlags vka::texture::get_usage() const
{
    return m_CreateInfo.usage;
}

vka::texture* vka::texture::set_usage(vk::ImageUsageFlags flags)
{
    m_CreateInfo.usage = flags;
    return this;
}

void vka::texture::create()
{
    auto device = get_device();

    if(m_Image)
    {
        throw std::runtime_error("Image already created.");
    }

    if( m_CreateInfo.extent.depth * m_CreateInfo.extent.width * m_CreateInfo.extent.height == 0)
        throw std::runtime_error("Image extents not set. Use Texture::SetSize()");


    m_Image = device.createImage(m_CreateInfo);

    if(!m_Image)
    {
        throw std::runtime_error("Error creating image");
    }

    auto initial_layout = m_CreateInfo.initialLayout;

    auto & L = m_Layout;
    L.clear();
    for(uint32_t l =0 ; l < m_CreateInfo.arrayLayers;++l)
    {
        for(uint32_t m =0 ; m < m_CreateInfo.mipLevels;++m)
        {
            L[l][m] = initial_layout;
        }
    }

    LOG << "Texture Created" << ENDL;

    //============= Allocate the Memory =================

    auto req = device.getImageMemoryRequirements(m_Image);
    m_Memory.allocate(req);
    m_Memory.bind(this);

    LOG << "Memory allocated: " << m_Memory << ENDL;

    //============== Create Image View ====================

    create_sampler();

}


void vka::texture::create_image_view( vk::ImageAspectFlags flags)
{
    vk::ImageViewCreateInfo create_info;

    create_info.image    = m_Image;

    if( m_CreateInfo.imageType != vk::ImageType::e2D )
    {
        throw std::runtime_error("Only 2D textures supported at the moment.");
    }

    create_info.viewType                        = get_view_type();// vk::ImageViewType::e2D;
    create_info.format                          = m_CreateInfo.format;
    create_info.subresourceRange.aspectMask     = flags;
    create_info.subresourceRange.baseMipLevel   = 0;
    create_info.subresourceRange.levelCount     = get_mipmap_levels();
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount     = get_layers();

    create_image_view( create_info );

}

void vka::texture::create_sampler()
{
    vk::SamplerCreateInfo create_info;

    create_info.magFilter        = vk::Filter::eLinear;// VK_FILTER_LINEAR;
    create_info.minFilter        = vk::Filter::eLinear;// VK_FILTER_LINEAR;
    create_info.addressModeU     = vk::SamplerAddressMode::eRepeat;//VK_SAMPLER_ADDRESS_MODE_REPEAT;
    create_info.addressModeV     = vk::SamplerAddressMode::eRepeat;//VK_SAMPLER_ADDRESS_MODE_REPEAT;
    create_info.addressModeW     = vk::SamplerAddressMode::eRepeat;//VK_SAMPLER_ADDRESS_MODE_REPEAT;
    create_info.anisotropyEnable = VK_FALSE;
    create_info.maxAnisotropy    = 1;
    create_info.borderColor      = vk::BorderColor::eIntOpaqueBlack;// VK_BORDER_COLOR_INT_OPAQUE_BLACK ;
    create_info.unnormalizedCoordinates = VK_FALSE;
    create_info.compareEnable    = VK_FALSE;
    create_info.compareOp        = vk::CompareOp::eAlways;// VK_COMPARE_OP_ALWAYS;
    create_info.mipmapMode       = vk::SamplerMipmapMode::eLinear;// VK_SAMPLER_MIPMAP_MODE_LINEAR;
    create_info.mipLodBias       = 0.0f;
    create_info.minLod           = 0.0f;
    create_info.maxLod           = (float)get_mipmap_levels();

    create_sampler(create_info);
}




void vka::texture::create_sampler(const vk::SamplerCreateInfo &create_info)
{
    m_SamplerInfo = create_info;

    if(m_Sampler)
        get_device().destroySampler(m_Sampler);

    m_Sampler = get_device().createSampler( m_SamplerInfo );
}

void vka::texture::create_image_view( const vk::ImageViewCreateInfo & view_info)
{
    if( m_View)
        throw std::runtime_error("Image View alreayd created. cannot create one again for this texture");

    m_ViewInfo = view_info;

    m_View = get_device().createImageView( m_ViewInfo );

    if ( !m_View )
    {
        throw std::runtime_error("Failed to create texture image view!");
    }
    LOG << "Image View Created" << ENDL;
}

uint32_t vka::texture::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{

    auto physicaldevice = get_physical_device();

    //auto memRequirements      = device().getBufferMemoryRequirements(m_Buffer);
    auto memProperites        = physicaldevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProperites.memoryTypeCount ; i++)
    {
        // vk::MemoryPropertyFlagBits properties = static_cast<vk::MemoryPropertyFlagBits>(Properties);
        if ((typeFilter & (1 << i)) && ( static_cast<vk::MemoryPropertyFlags>(memProperites.memoryTypes[i].propertyFlags) & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}


bool vka::texture::has_stencil_component(vk::Format format)
{
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

vk::ImageLayout vka::texture::get_layout(uint32_t layer, uint32_t mip_level) const
{
    return m_Layout.at(layer).at(mip_level);
}

void vka::texture::convert_layer(vk::CommandBuffer commandBuffer,
                                 vk::ImageLayout layout ,
                                 uint32_t layer, uint32_t level,
                                 vk::PipelineStageFlags srcStageMask,
                                 vk::PipelineStageFlags dstStageMask)
{

    vk::ImageSubresourceRange subresourceRange;
    if( layout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
    {
        subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        if( has_stencil_component( get_format() ) )
        {
            subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
    }
    else
    {
        subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    subresourceRange.baseMipLevel   = level;
    subresourceRange.levelCount     = 1;

    subresourceRange.baseArrayLayer = layer;
    subresourceRange.layerCount     = 1;

    convert(commandBuffer, get_layout(layer,level), layout, subresourceRange,srcStageMask, dstStageMask);
    m_Layout.at(layer).at(level) = layout;
}


void vka::texture::convert(vk::CommandBuffer commandBuffer,
                           vk::ImageLayout new_layout,
                           vk::PipelineStageFlags srcStageMask,
                           vk::PipelineStageFlags dstStageMask)
{

    //========= Check if all the layers are the same===========================
    auto current_layout = get_layout(0,0);
    for(uint32_t layer=0; layer < get_layers() ;layer++)
    {
        for(uint32_t level=0; level < get_mipmap_levels() ; level++)
        {
            auto L = get_layout(layer,level);
            if(L != current_layout)
            {
                throw std::runtime_error("Some layers/mipmap levels are not the same.");
            }
        }
    }


    vk::ImageSubresourceRange subresourceRange;

    if( new_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
    {
        subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        if( has_stencil_component( get_format() ) )
        {
            subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
    }
    else
    {
        subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    subresourceRange.baseMipLevel   = 0;
    subresourceRange.levelCount     = get_mipmap_levels();

    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount     = get_layers();

    convert(commandBuffer, current_layout, new_layout, subresourceRange,srcStageMask, dstStageMask);

    for(uint32_t layer=0; layer < get_layers() ;layer++)
    {
        for(uint32_t level=0; level < get_mipmap_levels() ; level++)
        {
            m_Layout.at(layer).at(level) = new_layout;
        }
    }



}


void vka::texture::convert( vk::CommandBuffer commandBuffer,
                            vk::ImageLayout old_layout ,
                            vk::ImageLayout new_layout ,
                            vk::ImageSubresourceRange const & range,
                            vk::PipelineStageFlags srcStageMask,
                            vk::PipelineStageFlags dstStageMask)
{
    vk::ImageMemoryBarrier barrier;

    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;

    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = m_Image;//image();

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

    commandBuffer.pipelineBarrier( srcStageMask,
                                   dstStageMask,
                                   vk::DependencyFlags(),0,0,barrier);



}

void vka::texture::convert(vk::ImageLayout new_layout, vk::PipelineStageFlags srcStageMask, vk::PipelineStageFlags dstStageMask)
{
    auto cb = get_parent_context()->get_command_pool()->AllocateCommandBuffer();

    cb.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );


    convert( cb, new_layout, srcStageMask, dstStageMask);

    cb.end();

    get_parent_context()->submit_cmd_buffer(cb);
}


void* vka::texture::map_memory()
{
    return  m_Memory.map();
//    if( m_Mapped )
//    {
//        return m_Mapped;
//    }
//    void * data = get_device().mapMemory( m_Memory, 0, m_MemoryRequirements.size, vk::MemoryMapFlags());
//    m_Mapped = data;
//    return data;
}

void  vka::texture::unmap_memory()
{
    m_Memory.unmap();
    //if( m_Mapped)
    //{
    //    get_device().unmapMemory( m_Memory );
    //    m_Mapped = nullptr;
    //}
}





//void vka::texture::copy_buffer( vka::buffer const * i, vk::BufferImageCopy const & region) const
void vka::texture::copy_buffer( vka::buffer const * b , vk::BufferImageCopy const region)
{
    auto commandBuffer = get_parent_context()->get_command_pool()->AllocateCommandBuffer(vk::CommandBufferLevel::ePrimary);

    commandBuffer.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );

    commandBuffer.copyBufferToImage( *b,
                                     get_image(),
                                     vk::ImageLayout::eTransferDstOptimal,
                                     region);

    commandBuffer.end();

    get_parent_context()->submit_cmd_buffer(commandBuffer);

    get_device().freeCommandBuffers( *get_parent_context()->get_command_pool() , commandBuffer );

}























void vka::texture2d::copy( vk::CommandBuffer cm, texture2d* t, vk::Offset2D dstOffset, vk::Offset2D srcOffset, vk::Extent2D extent)
{
    if( t->get_layout(0,0) != vk::ImageLayout::eTransferSrcOptimal )
    {
         t->convert_layer(cm, vk::ImageLayout::eTransferSrcOptimal,0,0);
    }
    if( get_layout(0,0) != vk::ImageLayout::eTransferDstOptimal )
    {
         this->convert_layer(cm, vk::ImageLayout::eTransferDstOptimal,0,0);
    }

    vk::ImageCopy IC;
    vk::ImageSubresourceLayers subResource;
    subResource.aspectMask     = vk::ImageAspectFlagBits::eColor;//  VK_IMAGE_ASPECT_COLOR_BIT;
    subResource.baseArrayLayer = 0;
    subResource.mipLevel       = 0;
    subResource.layerCount     = 1;
    IC.setDstOffset( vk::Offset3D(dstOffset.x,dstOffset.y,0))
      .setSrcOffset( vk::Offset3D(srcOffset.x,srcOffset.y,0))
      .setExtent(    vk::Extent3D(extent.width,extent.height,1))
      .setSrcSubresource(subResource)
      .setDstSubresource(subResource);

    cm.copyImage( t->get_image(),
                   vk::ImageLayout::eTransferSrcOptimal ,
                   get_image(),
                   vk::ImageLayout::eTransferDstOptimal,
                   IC);

}

/**
 * @brief copy
 * @param t
 *
 * Copies the texture, t, into this texture.  This method will
 * allocate it's own comamnd buffer and execture the commands.
 */
void vka::texture2d::copy( texture2d * t)
{

}

