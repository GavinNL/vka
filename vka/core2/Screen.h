#ifndef VKA_SCREEN2_TARGET_H
#define VKA_SCREEN2_TARGET_H

#include <vka/core/context_child.h>
#include <vka/core2/TextureMemoryPool.h>
#include <vka/core/types.h>


namespace vka
{

class CommandBuffer;
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
            setClearColorValue( vk::ClearColorValue( std::array<float,4>({r,g,b,a}) ));
        }
        void setClearColorValue( vk::ClearColorValue C);
        void setClearDepthValue( vk::ClearDepthStencilValue C);

       std::array<vk::ClearValue, 2> const & getClearValues() const;

        vk::Extent2D getExtent() const;


        vk::RenderPass getRenderPass() const;

        vk::Framebuffer getFramebuffer(uint32_t index) const;

        /**
         * @brief Create
         * @param surface
         * @param extent
         * @param depth_format
         *
         * Creates the Screen object.
         */
        void create( vk::SurfaceKHR surface,
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
        uint32_t getNextFrameIndex( vka::Semaphore_p & signal_semaphore);

        /**
         * @brief PresentFrame
         * @param frame_index - the frame index to present to the screen
         * @param wait_semaphore - the semaphore that must be waited on before
         *                         the frame is presented to the screen
         */
        void presentFrame(uint32_t frame_index, vka::Semaphore_p &wait_semaphore);
protected:

        static void                         createSwapchain(   vk::PhysicalDevice pd, vk::Device device, SwapChainData & SC, vk::SurfaceKHR surface, const vk::Extent2D &extent, bool vsync = false);
        static vk::RenderPass               createRenderPass(  vk::Device device, vk::Format swapchain_format, vk::Format depth_format);
        static std::vector<vk::Framebuffer> createFrameBuffers(vk::Device device, const vk::Extent2D &extent, vk::RenderPass renderpass, const std::vector<vk::ImageView> &swapchain_views, vk::ImageView depth_view);


        static vk::SurfaceFormatKHR         getSurfaceFormats(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface);
};

}


#endif
