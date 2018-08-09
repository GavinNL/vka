#ifndef VKA_SCREEN2_TARGET_H
#define VKA_SCREEN2_TARGET_H

#include <vka/core/context_child.h>
#include <vka/core2/TextureMemoryPool.h>
#include <vka/core/types.h>


namespace vka
{

class command_buffer;
class semaphore;



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

        vk::RenderPass                    m_renderpass;

        std::array<vk::ClearValue, 2>     m_clear_values;

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

       std::array<vk::ClearValue, 2> const & GetClearValues() const
       {
           return m_clear_values;
       }
       // void set_surface(vk::SurfaceKHR surface);
       //void set_extent(vk::Extent2D e);

        vk::Extent2D GetExtent() const
        {
            return m_extent;
        }


        vk::RenderPass GetRenderPass() const
        {
            return m_renderpass;
        }

        vk::Framebuffer GetFramebuffer(uint32_t index) const
        {
            return m_Swapchain.framebuffer.at(index);
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
            m_extent = extent;

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


        uint32_t GetNextFrameIndex(vka::semaphore *signal_semaphore);
        void PresentFrame(uint32_t frame_index, vka::semaphore * wait_semaphore);
protected:

        static void                         CreateSwapchain(   vk::PhysicalDevice pd, vk::Device device, SwapChainData & SC, vk::SurfaceKHR surface, const vk::Extent2D &extent, bool vsync = false);
        static vk::RenderPass               CreateRenderPass(  vk::Device device, vk::Format swapchain_format, vk::Format depth_format);
        static std::vector<vk::Framebuffer> CreateFrameBuffers(vk::Device device, const vk::Extent2D &extent, vk::RenderPass renderpass, const std::vector<vk::ImageView> &swapchain_views, vk::ImageView depth_view);


        static vk::SurfaceFormatKHR         GetSurfaceFormats(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface);
};

}


#endif
