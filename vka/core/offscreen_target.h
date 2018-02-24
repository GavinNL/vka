#ifndef VKA_OFF_SCREEN_TARGET_H
#define VKA_OFF_SCREEN_TARGET_H

#include "context_child.h"
#include "context.h"

namespace vka
{

class command_buffer;

class offscreen_target : public context_child
{
    protected:
        vk::Extent2D m_size;
        vka::renderpass     * m_renderpass = nullptr;
        vka::framebuffer    * m_framebuffer;

        std::vector<vka::texture*> m_attachments;
        std::vector<vk::ClearValue> m_clear_values;

        uint32_t m_depth_index; // the index in the above vector that is the depth attachment;

        vk::RenderPassBeginInfo m_renderpass_info;

        offscreen_target(context * parent);

    public:
        offscreen_target* set_extents( vk::Extent2D size);
        offscreen_target* add_color_attachment( vk::Extent2D size, vk::Format format);
        offscreen_target* add_depth_attachment( vk::Extent2D size, vk::Format format);

        renderpass* get_renderpass()
        {
            return m_renderpass;
        }

        framebuffer* get_framebuffer()
        {
            return m_framebuffer;
        }

        vka::texture* get_image(uint32_t i)
        {
            return m_attachments.at(i);
        }


        vk::ClearValue & clear_value(uint32_t i) { return m_clear_values.at(i); }

        void beginRender(vka::command_buffer & cb);
        void endRender(vka::command_buffer & cb);

        void create();
        friend class context;
        friend class deleter<offscreen_target>;
};

}


#endif
