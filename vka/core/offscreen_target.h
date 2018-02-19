#ifndef VKA_OFF_SCREEN_TARGET_H
#define VKA_OFF_SCREEN_TARGET_H

#include "render_target.h"
#include "context.h"

namespace vka
{

class offscreen_target : public context_child
{
    protected:
        vk::Extent2D m_size;
        vka::renderpass     * m_renderpass = nullptr;
        vka::framebuffer    * m_framebuffer;

        std::vector<vka::texture*> m_attachments;

        offscreen_target(context * parent);

    public:
        offscreen_target* set_extents( vk::Extent2D size);
        offscreen_target* add_color_attachment( vk::Extent2D size, vk::Format format);
        offscreen_target* add_depth_attachment( vk::Extent2D size, vk::Format format);
        void create();
        friend class context;
        friend class deleter<offscreen_target>;
};

}


#endif
