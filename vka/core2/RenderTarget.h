#pragma once
#ifndef VKA_RENDER_TARGET_3_H
#define VKA_RENDER_TARGET_3_H

#include "TextureMemoryPool.h"
#include <vka/core/types.h>
#include <vulkan/vulkan.hpp>

namespace vka
{

class RenderTarget : public context_child
{
public:
    RenderTarget(context * parent) :
        context_child(parent),
        m_ColorPool(parent),
        m_DepthPool(parent)
    {

    }
    ~RenderTarget()
    {
        clear();
    }

    void clear();

    RenderTarget & operator = ( RenderTarget && other)
    {
        if( this != & other)
        {

        }
        return *this;
    }

    void SetExtent( vk::Extent2D const & E)
    {
        m_Extent = E;
    }

    /**
     * @brief SetClearColorValue
     * @param i
     * @param r
     * @param g
     * @param b
     * @param a
     *
     * Sets the clear colour value for the particular render texture
     * Default is 0,0,0,0
     */
    void SetClearColorValue(uint32_t i, float r, float g, float b, float a);

    /**
     * @brief SetClearDepthValue
     * @param v
     *
     * Sets the clear depth value. If this is not set, it is defaulted to 1.0
     */
    void SetClearDepthValue(float v);

    /**
     * @brief SetClearStencilValue
     * @param v
     *
     * Sets the clear Stencil value
     */
    void SetClearStencilValue(float v);
    /**
     * @brief Create
     * @param color_formats - a vector of color formats to use, one for each render image
     * @param depth_format - the depth format to use for the depth texture
     *
     * Create a RenderTarget.
     */
    void Create( std::vector<vk::Format> const & color_formats, vk::Format depth_format);

    //==========================================================

    vk::RenderPass GetRenderPass() const
    {
        return m_RenderPass;
    }

    vk::Framebuffer GetFramebuffer() const
    {
        return m_Framebuffer;
    }

    Texture_p GetColorImage(uint32_t i) const
    {
        return m_images.at(i);
    }

    Texture_p GetDepthImage() const
    {
        return m_depth_image;
    }

    std::vector<vk::ClearValue> const & GetClearValues() const
    {
        return m_ClearValues;
    }

    vk::Extent2D GetExtent() const
    {
        return m_Extent;
    }
protected:
    TextureMemoryPool           m_ColorPool;
    TextureMemoryPool           m_DepthPool;

    vk::RenderPass              m_RenderPass;
    vk::Framebuffer             m_Framebuffer;

    std::vector<Texture_p>      m_images;
    Texture_p                   m_depth_image;

    uint32_t                    m_depth_idex = -1;
    std::vector<vk::ClearValue> m_ClearValues;

    vk::Extent2D                m_Extent;
};

}

#endif
