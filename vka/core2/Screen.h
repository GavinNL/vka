#ifndef VKA_SCREEN2_TARGET_H
#define VKA_SCREEN2_TARGET_H

#include <vka/core/context_child.h>
#include <vka/core2/TextureMemoryPool.h>
#include <vka/core/types.h>

namespace vka
{

class command_buffer;




struct SwapChainData
{
    vk::SwapchainKHR             swapchain;
    std::vector<vk::Image>       image;
    std::vector<vk::ImageView>   view;
    std::vector<vk::Framebuffer> framebuffer;
    vk::SurfaceFormatKHR         format;
};

class Screen : public context_child
{
    protected:
        vk::Extent2D m_extent;

        //vk::SurfaceKHR                    m_surface;

        //vk::Format                        m_image_format;
        vk::RenderPass                    m_renderpass;

        //vk::Format                        m_depth_format;
        //std::vector<vk::Image>            m_images;
        //std::vector<vk::ImageView>        m_image_views;

        std::array<vk::ClearValue, 2>     m_clear_values;

        //std::vector<SwapChainBuffer>      m_buffers;
        SwapChainData                     m_Swapchain;

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


       // void set_surface(vk::SurfaceKHR surface);
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

            auto physical_device = get_physical_device();
            auto device = get_device();

            //set_surface(surface);
            set_extent(extent);

            CreateSwapchain( physical_device, device, m_Swapchain, surface, extent,true);
            m_renderpass = CreateRenderPass(device, m_Swapchain.format.format, depth_format);

            //--------
            auto size = format_size(depth_format) * extent.width * extent.height;

            m_DepthPool.SetUsage( vk::ImageUsageFlagBits::eDepthStencilAttachment  |
                                  vk::ImageUsageFlagBits::eSampled);
            m_DepthPool.SetSize( size + 1024 );
            m_DepthImage = m_DepthPool.AllocateDepthAttachment( m_extent , depth_format);

            depth_format = m_DepthImage->GetFormat();
            //----------
            m_Swapchain.framebuffer = CreateFrameBuffers(device, extent, m_renderpass, m_Swapchain.view, m_DepthImage->GetImageView());
        }
protected:

        static void                         CreateSwapchain(   vk::PhysicalDevice pd, vk::Device device, SwapChainData & SC, vk::SurfaceKHR surface, const vk::Extent2D &extent, bool vsync = false);
        static vk::RenderPass               CreateRenderPass(  vk::Device device, vk::Format swapchain_format, vk::Format depth_format);
        static std::vector<vk::Framebuffer> CreateFrameBuffers(vk::Device device, const vk::Extent2D &extent, vk::RenderPass renderpass, const std::vector<vk::ImageView> &swapchain_views, vk::ImageView depth_view);


        static vk::SurfaceFormatKHR         GetSurfaceFormats(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface);
};

}


#endif
