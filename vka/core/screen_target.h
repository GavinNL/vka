#ifndef VKA_SCREEN_TARGET_H
#define VKA_SCREEN_TARGET_H

#include "context_child.h"
#include "context.h"

namespace vka
{

class command_buffer;

class screen : public context_child
{
    protected:
        vk::Extent2D m_extent;

        vk::SurfaceKHR                    m_surface;
        vk::SurfaceFormatKHR              m_swapchain_format;
        vk::SwapchainKHR                  m_swapchain;
        vk::Format                        m_image_format;
        std::vector<vk::Image>            m_images;
        std::vector<vk::ImageView>        m_image_views;

        std::array<vk::ClearValue, 2>     m_clear_values;

        vka::texture* m_depth_texture = nullptr;

        vka::renderpass     * m_renderpass = nullptr;

        vk::RenderPassBeginInfo m_renderpass_info;

        uint32_t m_next_frame_index;

        std::vector<vk::ImageView> create_image_views(const std::vector<vk::Image> &images, vk::Format image_format);
        std::vector<vka::framebuffer*> m_framebuffers;
        screen(context * parent);
        ~screen();


    public:


        friend class context;
        friend class deleter<screen>;

        void set_clear_color_value( vk::ClearColorValue C)
        {
            m_clear_values[0] = C;
        }
        void set_clear_depth_value( vk::ClearDepthStencilValue C)
        {
            m_clear_values[1] = C;
        }

        vka::framebuffer* get_framebuffer(uint32_t index)
        {
            return m_framebuffers.at(index);
        }
        void create();
        void set_surface(vk::SurfaceKHR surface);
        void set_extent(vk::Extent2D e);

        vka::renderpass* get_renderpass() { return m_renderpass; }

        vk::Extent2D get_extent() const
        {
            return m_extent;
        }
        /**
         * @brief prepare_next_frame
         * @param signal_semaphore
         * @return Index of the next frame
         *
         * Prepares the next frame in the swap chain for rendering,
         * Signal the semaphore when it is complete
         */
        uint32_t prepare_next_frame(vka::semaphore * signal_semaphore);

        /**
         * @brief present_frame
         * @param frame_index
         * @param wait_semaphore
         *
         * Present the frame to the screen, but wait on the semaphore
         */
        void present_frame(uint32_t frame_index, vka::semaphore * wait_semaphore);

        void beginRender(vka::command_buffer & cb, uint32_t frame_index);
        void endRender(vka::command_buffer & cb);
};

}


#endif
