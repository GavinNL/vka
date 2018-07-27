#pragma once
#ifndef VKA_TEXTURE_MEMORYPOOL_H
#define VKA_TEXTURE_MEMORYPOOL_H


// ========= Standard Library ==========
#include <map>
#include <cmath>

// =========  Standard Vulkan =============
#include <vulkan/vulkan.hpp>


#include "Memory.h"

#include <vka/core/log.h>

#include <vka/utils/buffer_memory_manager.h>

namespace vka
{

class TextureMemoryPool;
class command_buffer;

/**
 * @brief The SubBuffer class
 *
 * A subbuffer represents a portion of the TextureMemoryPool
 *
 */
class Texture
{
private:
    Texture()
    {
    }
    public:

    ~Texture()
    {
        Destroy();
    }

    Texture(Texture & other) = delete;
    Texture & operator=(Texture & other) = delete;

    vk::DeviceSize GetOffset() const
    {
        return m_offset;
    }

    vk::DeviceSize GetSize() const
    {
        return m_size;
    }

    vk::Image GetImage() const
    {
        return m_Image;
    }

    vk::Extent3D const & GetExtents() const
    {
        return m_create_info.extent;
    }

    vk::DeviceSize GetArrayLayers() const
    {
        return m_create_info.arrayLayers;
    }

    vk::DeviceSize GetMipLevels() const
    {
        return m_create_info.mipLevels;
    }

    vk::ImageLayout GetLayout(uint32_t MipLevel = 0, uint32_t ArrayLayer = 0)
    {
        return m_LayoutsA.at(ArrayLayer).at(MipLevel);
    }


    void Destroy();




    vk::Format GetFormat() const
    {
        return m_create_info.format;
    }

    /**
     * @brief GetImageView
     * @param name
     * @return
     *
     * Returns the image view with the given name.
     */
    vk::ImageView GetImageView(const std::string & name = "default") const
    {
        return m_Views.at(name);
    }

    void CreateImageView( std::string const & name,
                          vk::ImageViewType view_type,
                          vk::ImageAspectFlags flags,
                          uint32_t base_array_layer, uint32_t num_array_layers,
                          uint32_t base_mip_level, uint32_t num_mip_levels)
    {
        vk::ImageViewCreateInfo C;
        C.image                           = m_Image;
        C.viewType                        = view_type;
        C.format                          = GetFormat();
        C.subresourceRange.aspectMask     = flags;
        C.subresourceRange.baseMipLevel   = base_mip_level;
        C.subresourceRange.levelCount     = num_mip_levels;
        C.subresourceRange.baseArrayLayer = base_array_layer;
        C.subresourceRange.layerCount     = num_array_layers;

        CreateImageView(name, C);
    }


    vk::SamplerCreateInfo GetDefaultSamplerCreateInfo() const
    {
        vk::SamplerCreateInfo SamplerInfo;
        SamplerInfo.magFilter        = vk::Filter::eLinear;// VK_FILTER_LINEAR;
        SamplerInfo.minFilter        = vk::Filter::eLinear;// VK_FILTER_LINEAR;
        SamplerInfo.addressModeU     = vk::SamplerAddressMode::eRepeat;//VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SamplerInfo.addressModeV     = vk::SamplerAddressMode::eRepeat;//VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SamplerInfo.addressModeW     = vk::SamplerAddressMode::eRepeat;//VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SamplerInfo.anisotropyEnable = VK_TRUE;
        SamplerInfo.maxAnisotropy    = 1;
        SamplerInfo.borderColor      = vk::BorderColor::eIntOpaqueBlack;// VK_BORDER_COLOR_INT_OPAQUE_BLACK ;
        SamplerInfo.unnormalizedCoordinates = VK_FALSE;
        SamplerInfo.compareEnable    = VK_FALSE;
        SamplerInfo.compareOp        = vk::CompareOp::eAlways;// VK_COMPARE_OP_ALWAYS;
        SamplerInfo.mipmapMode       = vk::SamplerMipmapMode::eLinear;// VK_SAMPLER_MIPMAP_MODE_LINEAR;
        SamplerInfo.mipLodBias       = 0.0f;
        SamplerInfo.minLod           = 0.0f;
        SamplerInfo.maxLod           = GetMipLevels();
        return SamplerInfo;
    }
    /**
     * @brief CreateImageView
     * @param name - name to give the view
     * @param CreateInfo
     *
     * Create an ImageView using the CreateInfo struct. The View can be
     * retrieved by using the GetImageView(name)
     */
    void CreateImageView(const std::string & name, vk::ImageViewCreateInfo CreateInfo);




    vk::Sampler GetSampler( const std::string & name = "default") const
    {
        return m_Samplers.at(name);
    }

    /**
     * @brief CreateSampler
     * @param name
     * @param CreateInfo
     *
     * Create a sampler with a given name
     */
    void CreateSampler(const std::string & name, vk::SamplerCreateInfo const & CreateInfo);


    void DestroyImageView(vk::ImageView V);
    void DestroySampler(vk::Sampler S);
    void DestroyImageView(const std::string & name = "default");
    void DestroySampler(const std::string & name = "default");

    protected:
        TextureMemoryPool *   m_parent = nullptr;

        vk::Image             m_Image;

        vk::ImageCreateInfo   m_create_info;
        vk::DeviceSize        m_offset=0; // memory offset . are these needed?
        vk::DeviceSize        m_size=0;   // memory size   . are these needed?

        std::vector< std::vector<vk::ImageLayout > > m_LayoutsA;

        std::map<std::string, vk::ImageView> m_Views;
        std::map<std::string, vk::Sampler>   m_Samplers;

        friend class TextureMemoryPool;
        friend class command_buffer;
};

using Texture_p = std::shared_ptr<Texture>;


/**
 * @brief The TextureMemoryPool class
 *
 * The TextureMemoryPool class is used to allocate textures.
 * There should only be one or two TextureMemory pools and they should be allocated
 * to have a large amount of space 50+MB.
 *
 */
class TextureMemoryPool : public context_child
{
public:

    TextureMemoryPool(context * parent) : context_child(parent) ,
                                         m_memory(parent)
    {

        m_CreateInfo.imageType     = vk::ImageType::e2D;// VK_IMAGE_TYPE_2D;

        m_CreateInfo.extent.width  = 0;
        m_CreateInfo.extent.height = 0;
        m_CreateInfo.extent.depth  = 0;
        m_CreateInfo.mipLevels     = 0;
        m_CreateInfo.arrayLayers   = 0;

        m_CreateInfo.format        = vk::Format::eR8G8B8A8Unorm;
        m_CreateInfo.tiling        = vk::ImageTiling::eLinear;

        m_CreateInfo.initialLayout = vk::ImageLayout::eUndefined;

        m_CreateInfo.usage         = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;// VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        m_CreateInfo.samples       = vk::SampleCountFlagBits::e1; // VK_SAMPLE_COUNT_1_BIT;
        m_CreateInfo.sharingMode   = vk::SharingMode::eExclusive; // VK_SHARING_MODE_EXCLUSIVE;


        m_MemoryRequirements.size = -1;
    }

    ~TextureMemoryPool()
    {
        Destroy();
    }

    TextureMemoryPool* SetTiling(vk::ImageTiling t)
    {
        m_CreateInfo.tiling = t;
        return this;
    }

    TextureMemoryPool* SetUsage(vk::ImageUsageFlags flags)
    {
        m_CreateInfo.usage = flags;
        return this;
    }

    TextureMemoryPool* SetSize(std::size_t size)
    {
        m_size = size;
        //m_create_info.size = size;
        return this;
    }

    TextureMemoryPool* SetMemoryProperties(vk::MemoryPropertyFlags flags)
    {
        m_memory.SetMemoryProperties(flags);
        return this;
    }


    void Destroy()
    {
      //  auto device = get_device();
      //  if(m_buffer)
      //  {
      //      device.destroyBuffer(m_buffer);
      //      m_buffer = vk::Buffer();
      //      LOG << "TextureMemoryPool destroyed" << ENDL;
      //  }
    }


    /**
     * @brief AllocateTexture
     * @param format - format
     * @param extent - size of the texture
     * @param arrayLayers - number of array layers
     * @param mipLevels - number of mipmap levels
     * @param tiling - image tiling
     * @param sharingMode
     * @return
     *
     * Allocate a texture from the memory pool. This is a generic
     * allocation method and can be used to allocate any type of
     * texture.
     *
     * An ImageView and a Sampler are NOT created using these functions.
     *
     */
    std::shared_ptr<Texture> AllocateTexture( vk::Format   format,
                                              vk::Extent3D extent,
                                              uint32_t     arrayLayers,
                                              uint32_t     mipLevels,
                                              vk::ImageTiling tiling,
                                              vk::SharingMode sharingMode)
    {
        auto device = get_device();

        assert(m_size != 0);

        vk::ImageCreateInfo create_info = m_CreateInfo;
        create_info.extent              = extent;
        create_info.arrayLayers         = arrayLayers;
        create_info.format              = format;
        create_info.mipLevels           = mipLevels;
        create_info.tiling              = tiling;
        create_info.sharingMode         = sharingMode;

        auto image = device.createImage( create_info );

        if( image )
        {
            vk::MemoryRequirements req = device.getImageMemoryRequirements(image);
            LOG << "===========================================" << ENDL;
            LOG << "MemoryRequirements: " << ENDL;
            LOG << "   Alignment      : " << req.alignment << ENDL;
            LOG << "   Size           : " << req.size << ENDL;
            LOG << "   Size           : " << (double)req.size/(1024*1024) << ENDL;
            LOG << "   TypeBits       : " << req.memoryTypeBits << ENDL;
            LOG << "===========================================" << ENDL;

            if( m_MemoryRequirements.size != (vk::DeviceSize)(-1) )
            {
               // Memory has already been allocated! Make sure that the
               // memory required to store this type of image is
               // the same as the one already created.

                assert( m_MemoryRequirements.alignment == req.alignment );
                //assert( m_MemoryRequirements.memoryTypeBits== req.memoryTypeBits);
            }
            else
            {
                // Memory hasn't been created yet.
                m_MemoryRequirements = req;
                m_MemoryRequirements.size = m_size;
                m_memory.Allocate(m_MemoryRequirements);

                m_manager.reset( m_size );

            }

            // find the offset into the memory where we can store
            // data for this image.
            auto offset = m_manager.allocate( req.size, req.alignment);

            m_memory.Bind( image, offset );

            std::shared_ptr<Texture> T( new Texture() );
            T->m_create_info = create_info;
            T->m_parent      = this;
            T->m_Image       = image;
            T->m_offset      = offset;
            T->m_size        = req.size;


            std::vector<vk::ImageLayout> mips;
            mips.assign(mipLevels, vk::ImageLayout::eUndefined);

            T->m_LayoutsA.assign(arrayLayers, mips);

            return T;
        }

        return nullptr;
    }

    void FreeTexture( Texture & S )
    {
        m_manager.free( S.m_offset);
        S.m_offset = 0;
        S.m_size = 0;
        S.m_parent = nullptr;

        get_device().destroyImage(S.m_Image);
        S.m_Image = vk::Image();
    }


    /**
     * @brief AllocateTexture2D
     * @param format
     * @param extent
     * @param arrayLayers
     * @param mipLevels
     * @param sharingMode
     * @return
     *
     * Allocate a 2D texture or a 2D Array. A "default" ImageView is created
     * with is a view into the entire texture as well as a "default" sampler.
     */
    std::shared_ptr<Texture> AllocateTexture2D( vk::Format format,
                                                vk::Extent2D extent,
                                                uint32_t     arrayLayers=1,
                                                uint32_t     mipLevels=std::numeric_limits<uint32_t>::max(),
                                                vk::SharingMode sharingMode=vk::SharingMode::eExclusive)
    {
        if( mipLevels ==std::numeric_limits<uint32_t>::max() )
        {
            mipLevels = std::min( std::log2( extent.height), std::log2( extent.width) );
        }

        auto T = AllocateTexture( format,
                                  vk::Extent3D{extent.width,extent.height,1},
                                  arrayLayers,
                                  mipLevels,
                                  vk::ImageTiling::eOptimal,
                                  sharingMode);

        T->CreateImageView( "default",
                            arrayLayers==1?vk::ImageViewType::e2D : vk::ImageViewType::e2DArray,
                            vk::ImageAspectFlagBits::eColor,
                            0, arrayLayers,
                            0, mipLevels);

        T->CreateSampler("default", T->GetDefaultSamplerCreateInfo() );

        return T;
    }

    std::shared_ptr<Texture> AllocateColorAttachment( vk::Format format,
                                                      vk::Extent2D extent)
    {
        auto T = AllocateTexture( format,
                                  vk::Extent3D{extent.width,extent.height,1},
                                  1,
                                  1,
                                  vk::ImageTiling::eOptimal,
                                  vk::SharingMode::eExclusive);

        T->CreateImageView( "default",
                            vk::ImageViewType::e2D,
                            vk::ImageAspectFlagBits::eColor,
                            0, 1,
                            0, 1);

        auto SCI = T->GetDefaultSamplerCreateInfo();
        SCI.magFilter        = vk::Filter::eNearest;// VK_FILTER_LINEAR;
        SCI.minFilter        = vk::Filter::eNearest;// VK_FILTER_LINEAR;
        SCI.addressModeU     = vk::SamplerAddressMode::eClampToEdge;//VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SCI.addressModeV     = vk::SamplerAddressMode::eClampToEdge;//VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SCI.addressModeW     = vk::SamplerAddressMode::eClampToEdge;//VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SCI.anisotropyEnable = VK_TRUE;
        SCI.maxAnisotropy    = 1;
        SCI.borderColor      = vk::BorderColor::eFloatOpaqueWhite;// VK_BORDER_COLOR_INT_OPAQUE_BLACK ;
        SCI.unnormalizedCoordinates = VK_FALSE;
        SCI.compareEnable    = VK_FALSE;
        SCI.compareOp        = vk::CompareOp::eAlways;// VK_COMPARE_OP_ALWAYS;
        SCI.mipmapMode       = vk::SamplerMipmapMode::eLinear;// VK_SAMPLER_MIPMAP_MODE_LINEAR;
        SCI.mipLodBias       = 0.0f;
        SCI.minLod           = 0.0f;
        SCI.maxLod           = 1.0;
        T->CreateSampler("default", SCI );

        return T;
    }

    std::shared_ptr<Texture> AllocateDepthAttachment( vk::Extent2D extent,
                                                      vk::Format format = vk::Format::eD32Sfloat)
    {
        assert(
         ( format  == vk::Format::eD32Sfloat       ) ||
         ( format  == vk::Format::eD32SfloatS8Uint ) ||
         ( format  == vk::Format::eD24UnormS8Uint  ) );


        auto T = AllocateTexture( format,
                                  vk::Extent3D{extent.width,extent.height,1},
                                  1,
                                  1,
                                  vk::ImageTiling::eOptimal,
                                  vk::SharingMode::eExclusive);

        T->CreateImageView( "default",
                            vk::ImageViewType::e2D,
                            vk::ImageAspectFlagBits::eStencil| vk::ImageAspectFlagBits::eDepth,
                            0, 1,
                            0, 1);

        auto SCI = T->GetDefaultSamplerCreateInfo();
        SCI.magFilter        = vk::Filter::eNearest;// VK_FILTER_LINEAR;
        SCI.minFilter        = vk::Filter::eNearest;// VK_FILTER_LINEAR;
        SCI.addressModeU     = vk::SamplerAddressMode::eClampToEdge;//VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SCI.addressModeV     = vk::SamplerAddressMode::eClampToEdge;//VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SCI.addressModeW     = vk::SamplerAddressMode::eClampToEdge;//VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SCI.anisotropyEnable = VK_TRUE;
        SCI.maxAnisotropy    = 1;
        SCI.borderColor      = vk::BorderColor::eFloatOpaqueWhite;// VK_BORDER_COLOR_INT_OPAQUE_BLACK ;
        SCI.unnormalizedCoordinates = VK_FALSE;
        SCI.compareEnable    = VK_FALSE;
        SCI.compareOp        = vk::CompareOp::eAlways;// VK_COMPARE_OP_ALWAYS;
        SCI.mipmapMode       = vk::SamplerMipmapMode::eLinear;// VK_SAMPLER_MIPMAP_MODE_LINEAR;
        SCI.mipLodBias       = 0.0f;
        SCI.minLod           = 0.0f;
        SCI.maxLod           = 1.0;
        T->CreateSampler("default", SCI );

        return T;
    }
protected:
    vka::Memory                m_memory;
    vka::buffer_memory_manager m_manager;
    vk::DeviceSize             m_size = 0;
    vk::MemoryRequirements     m_MemoryRequirements;
    vk::ImageCreateInfo        m_CreateInfo;


};

inline void Texture::CreateImageView(const std::string & name, vk::ImageViewCreateInfo CreateInfo)
{
    if( m_Views.count(name) )
    {
        throw std::runtime_error("A view with that name already exists");
    }

    auto v = m_parent->get_device().createImageView(CreateInfo);
    if( v )
    {
        m_Views[name] = v;
        return;
    }
    throw std::runtime_error("Error Creating Image View");
}

inline void Texture::CreateSampler(const std::string &name, const vk::SamplerCreateInfo &CreateInfo)
{
    if( m_Samplers.count(name) )
    {
        throw std::runtime_error("A view with that name already exists");
    }

    auto sampler =  m_parent->get_device().createSampler( CreateInfo );
    if( !sampler )
    {
        throw std::runtime_error("Error creating sampler");
    }
    m_Samplers[name] = sampler;
}


inline void Texture::DestroyImageView(vk::ImageView V)
{
    for(auto & s : m_Views)
        if(s.second == V)
            DestroyImageView(s.first);
}

inline void Texture::DestroySampler(vk::Sampler S)
{
    for(auto & s : m_Samplers)
        if(s.second == S)
            DestroySampler(s.first);
}

inline void Texture::DestroyImageView(const std::string & name)
{
    auto v = m_Views.at(name);
    m_parent->get_device().destroyImageView(v);
    m_Views.erase(name);
}

inline void Texture::DestroySampler(const std::string & name )
{
    auto v = m_Samplers.at(name);
    m_parent->get_device().destroySampler(v);
    m_Samplers.erase(name);
}



inline void Texture::Destroy()
{
    if(m_Image)
    {
        for(auto & v : m_Views)
        {
            m_parent->get_device().destroyImageView( v.second );
        }
        for(auto & v : m_Samplers)
        {
            m_parent->get_device().destroySampler( v.second );
        }
        m_Samplers.clear();
        m_Views.clear();
        m_parent->FreeTexture(*this);
    }
}

}

#endif
