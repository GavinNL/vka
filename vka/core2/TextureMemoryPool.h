#pragma once
#ifndef VKA_TEXTURE_MEMORYPOOL_H
#define VKA_TEXTURE_MEMORYPOOL_H

#include <vulkan/vulkan.hpp>

#include "Memory.h"

#include <vka/core/log.h>

#include <vka/utils/buffer_memory_manager.h>

namespace vka
{

class TextureMemoryPool;

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

    void Destroy();

    protected:
        TextureMemoryPool *   m_parent = nullptr;

        vk::Image             m_Image;

        vk::ImageCreateInfo   m_create_info;
        vk::DeviceSize        m_offset=0;
        vk::DeviceSize        m_size=0;

        friend class TextureMemoryPool;
};




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

        m_CreateInfo.initialLayout = vk::ImageLayout::ePreinitialized;

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

    void Create()
    {
       // auto device = get_device();
       //
       // if( m_create_info.size == 0)
       // {
       //     throw std::runtime_error("Size not set! Please use the set_size( ) method.");
       // }
       // if( m_buffer )
       // {
       //     throw std::runtime_error("Buffer already created");
       // }
       //
       //
       // m_buffer = device.createBuffer( m_create_info );
       //
       // if(!m_buffer)
       //     throw std::runtime_error("Failed to create buffer");
       //
       // m_memory.Allocate( device.getBufferMemoryRequirements(m_buffer) );
       //
       // m_create_info.size = m_memory.GetSize();
       //
       // m_memory.Bind(m_buffer, 0);
       // m_manager.reset( m_create_info.size  );

    }

    std::shared_ptr<Texture> AllocateTexture( vk::Extent3D extent,
                                              uint32_t     arrayLayers,
                                              vk::Format   format,
                                              uint32_t     mipLevels,
                                              vk::ImageTiling tiling,
                                              vk::SharingMode sharingMode)
    {
        auto device = get_device();

        assert(m_size != 0);

        vk::ImageCreateInfo create_info = m_CreateInfo;
        create_info.extent = extent;
        create_info.arrayLayers = arrayLayers;
        create_info.format = format;
        create_info.mipLevels = mipLevels;
        create_info.tiling = tiling;
        create_info.sharingMode = sharingMode;

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
                assert( m_MemoryRequirements.memoryTypeBits== req.memoryTypeBits);
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
            return T;
        }

        return nullptr;
    }

protected:
    vka::Memory                m_memory;
    vka::buffer_memory_manager m_manager;
    vk::DeviceSize             m_size = 0;
    vk::MemoryRequirements     m_MemoryRequirements;
    vk::ImageCreateInfo        m_CreateInfo;


};


inline void Texture::Destroy()
{
    if(m_Image)
        m_parent->get_device().destroyImage(m_Image);
}

}

#endif
