#ifndef VKA_SCREEN2_TARGET_H
#define VKA_SCREEN2_TARGET_H

#include <vka/core/context_child.h>
#include <vka/core2/TextureMemoryPool.h>
#include <vka/core/types.h>


namespace vka
{

class command_buffer;
class Semaphore;
using Semaphore_p = std::shared_ptr<Semaphore>;


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

        void SetClearColorValue( float r, float g, float b, float a )
        {
            set_clear_color_value( vk::ClearColorValue( std::array<float,4>({r,g,b,a}) ));
        }
        void set_clear_color_value( vk::ClearColorValue C);
        void set_clear_depth_value( vk::ClearDepthStencilValue C);

       std::array<vk::ClearValue, 2> const & GetClearValues() const;

        vk::Extent2D GetExtent() const;


        vk::RenderPass GetRenderPass() const;

        vk::Framebuffer GetFramebuffer(uint32_t index) const;

        /**
         * @brief Create
         * @param surface
         * @param extent
         * @param depth_format
         *
         * Creates the Screen object.
         */
        void Create( vk::SurfaceKHR surface,
                     vk::Extent2D const & extent,
                     vk::Format depth_format = vk::Format::eD32Sfloat );


        /**
         * @brief GetNextFrameIndex
         * @param signal_semaphore - a semaphore that must be signaled when the image is actually ready
         * @return
         *
         * Returns the next available frame index which can be
         * drawn on.
         *
         * The signal semaphore
         */
        uint32_t GetNextFrameIndex( vka::Semaphore_p & signal_semaphore);

        /**
         * @brief PresentFrame
         * @param frame_index - the frame index to present to the screen
         * @param wait_semaphore - the semaphore that must be waited on before
         *                         the frame is presented to the screen
         */
        void PresentFrame(uint32_t frame_index, vka::Semaphore_p &wait_semaphore);
protected:

        static void                         CreateSwapchain(   vk::PhysicalDevice pd, vk::Device device, SwapChainData & SC, vk::SurfaceKHR surface, const vk::Extent2D &extent, bool vsync = false);
        static vk::RenderPass               CreateRenderPass(  vk::Device device, vk::Format swapchain_format, vk::Format depth_format);
        static std::vector<vk::Framebuffer> CreateFrameBuffers(vk::Device device, const vk::Extent2D &extent, vk::RenderPass renderpass, const std::vector<vk::ImageView> &swapchain_views, vk::ImageView depth_view);


        static vk::SurfaceFormatKHR         GetSurfaceFormats(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface);
};

}


#endif
