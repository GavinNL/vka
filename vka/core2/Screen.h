#ifndef VKA_SCREEN2_TARGET_H
#define VKA_SCREEN2_TARGET_H

#include <vka/core/context_child.h>
#include <vka/core2/TextureMemoryPool.h>

namespace vka
{

class command_buffer;

struct SwapChainBuffer
{
        vk::Image     image;
        vk::ImageView view;
        vk::Framebuffer framebuffer;
};

struct DepthImage
{
    vk::Image image;
    vk::ImageView view;
};

class Screen : public context_child
{
    protected:
        vk::Extent2D m_extent;

        vk::SurfaceKHR                    m_surface;

        vk::SurfaceFormatKHR              m_swapchain_format;
        vk::SwapchainKHR                  m_swapchain;

        vk::Format                        m_image_format;
        vk::RenderPass                    m_renderpass;

        vk::Format                        m_depth_format;
        std::vector<vk::Image>            m_images;
        std::vector<vk::ImageView>        m_image_views;

        std::array<vk::ClearValue, 2>     m_clear_values;

        std::vector<SwapChainBuffer>      m_buffers;


        TextureMemoryPool                 m_DepthPool;
        Texture_p                         m_DepthImage;


    public:
        friend class context;

        Screen(context * parent);
        ~Screen();

        void set_clear_color_value( vk::ClearColorValue C)
        {
            m_clear_values[0] = C;
        }
        void set_clear_depth_value( vk::ClearDepthStencilValue C)
        {
            m_clear_values[1] = C;
        }


        void set_surface(vk::SurfaceKHR surface);
        void set_extent(vk::Extent2D e);

        vk::Extent2D get_extent() const
        {
            return m_extent;
        }


        void Create(
                vk::SurfaceKHR surface,
                vk::Extent2D const & extent,
                vk::Format depth_format = vk::Format::eD32Sfloat
                )
        {
            set_surface(surface);
            set_extent(extent);
            m_depth_format = depth_format;

            CreateSwapchain(extent.width,extent.height,true);
            setupRenderPass();
            setupFrameBuffer();
        }
protected:

        void CreateSwapchain(uint32_t width, uint32_t height, bool vsync = false);
        vk::SurfaceFormatKHR GetSurfaceFormats(vk::SurfaceKHR surface);
        void setupRenderPass();
        void setupFrameBuffer();
};

}


#endif
