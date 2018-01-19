#ifndef VKA_PIPELINE_H
#define VKA_PIPELINE_H

#include <vulkan/vulkan.hpp>
#include "deleter.h"
#include "context_child.h"
#include "classes.h"
#include <map>

namespace vka
{



class pipeline : public context_child
{
private:
    vk::PipelineLayout      m_PipelineLayout;

    vk::Pipeline            m_pipeline;


    vk::Rect2D   m_scissor;
    vk::Viewport m_viewport;

    vk::PipelineDepthStencilStateCreateInfo  m_DepthStencil;

    vk::PipelineViewportStateCreateInfo      m_ViewportState;
    vk::PipelineRasterizationStateCreateInfo m_Rasterizer;

    vk::PipelineMultisampleStateCreateInfo   m_Multisampling;
    vk::PipelineColorBlendAttachmentState    m_ColorBlendAttachment;
    vk::PipelineColorBlendStateCreateInfo    m_ColorBlending;

    vk::PipelineVertexInputStateCreateInfo   m_VertexInputInfo;
    vk::PipelineInputAssemblyStateCreateInfo m_InputAssembly;


    std::vector<vk::VertexInputAttributeDescription>  m_VertexAttributeDescription;
    vk::VertexInputBindingDescription                 m_VertexBindDescription;

    std::vector<vk::PushConstantRange>                m_PushConstantRange;

    std::map<uint32_t , std::vector<vk::DescriptorSetLayoutBinding> >
                                                     m_DescriptorSetLayoutBindings;
    //std::vector<vka::descriptor_set_layout*>         m_DescriptorSetLayouts;

    vka::shader * m_VertexShader   = nullptr;
    std::string   m_VertexShaderEntry;
    vka::shader * m_FragmentShader = nullptr;
    std::string   m_FragmentShaderEntry;

    vka::renderpass  * m_RenderPass = nullptr;
    //std::vector<
    //vka::DescriptorSetLayout>                         m_DSetLayouts;


    pipeline(context * parent) : context_child(parent)
    {
     //   #warning We need to change this
        m_viewport = vk::Viewport{0.0f,0.0f, 640.0f, 480.0f, 0.0f, 1.0f};

        m_scissor.offset = vk::Offset2D{0, 0};
        m_scissor.extent = vk::Extent2D{640, 480};

        //==========
        m_ViewportState.viewportCount = 1;
        m_ViewportState.scissorCount  = 1;
        //==========

        m_Rasterizer.depthClampEnable        = VK_FALSE;
        m_Rasterizer.rasterizerDiscardEnable = VK_FALSE;
        m_Rasterizer.polygonMode             = vk::PolygonMode::eFill;// VK_POLYGON_MODE_FILL;
        m_Rasterizer.lineWidth               = 1.0f;
        m_Rasterizer.cullMode                = vk::CullModeFlagBits::eBack;// VK_CULL_MODE_BACK_BIT;
        //m_Rasterizer.cullMode                = vk::CullModeFlagBits::eNone;// VK_CULL_MODE_BACK_BIT;

       //  #warning we should change this to counter clockwise by default?
        m_Rasterizer.frontFace               = vk::FrontFace::eCounterClockwise;// VK_FRONT_FACE_CLOCKWISE;
        //m_Rasterizer.frontFace               = vk::FrontFace::eClockwise;// VK_FRONT_FACE_CLOCKWISE;
        m_Rasterizer.depthBiasEnable         = VK_FALSE;

        //==========

        m_Multisampling.sampleShadingEnable = VK_FALSE;
        m_Multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;// VK_SAMPLE_COUNT_1_BIT;

        //==========

        m_ColorBlendAttachment.colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        m_ColorBlendAttachment.blendEnable         = VK_TRUE;
        m_ColorBlendAttachment.colorBlendOp        = vk::BlendOp::eAdd;
        m_ColorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
        m_ColorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;

        m_ColorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusDstAlpha;
        m_ColorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusDstAlpha;

        //==========
        m_ColorBlending.logicOpEnable     = VK_FALSE;
        m_ColorBlending.logicOp           = vk::LogicOp::eCopy;
        m_ColorBlending.blendConstants[0] = 1.0f;
        m_ColorBlending.blendConstants[1] = 1.0f;
        m_ColorBlending.blendConstants[2] = 1.0f;
        m_ColorBlending.blendConstants[3] = 1.0f;
        m_ColorBlending.attachmentCount   = 1;
        m_ColorBlending.pAttachments      = &m_ColorBlendAttachment;
        //===========


        //==================
        m_DepthStencil.depthTestEnable       = VK_TRUE;
        m_DepthStencil.depthWriteEnable      = VK_TRUE;
        m_DepthStencil.depthCompareOp        = vk::CompareOp::eLess;// VK_COMPARE_OP_LESS;
        m_DepthStencil.depthBoundsTestEnable = VK_FALSE;
        m_DepthStencil.stencilTestEnable     = VK_FALSE;

        m_DepthStencil.minDepthBounds        = 0.0f; // Optional
        m_DepthStencil.maxDepthBounds        = 1.0f; // Optional
        m_DepthStencil.stencilTestEnable     = VK_FALSE;
        //m_DepthStencil.front             = {};
        //m_DepthStencil.back              = {};
        //==================

        m_VertexInputInfo.vertexBindingDescriptionCount   = 0;
        m_VertexInputInfo.vertexAttributeDescriptionCount = 0;


        m_ViewportState.pViewports    = &m_viewport;
        m_ViewportState.pScissors     = &m_scissor;


        m_InputAssembly.topology               = vk::PrimitiveTopology::eTriangleList;// VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        m_InputAssembly.primitiveRestartEnable = VK_FALSE;


    }

    ~pipeline();

public:

    operator vk::Pipeline()
    {
        return m_pipeline;
    }

    pipeline* set_scissor( vk::Rect2D const & sc)
    {
        m_scissor = sc;
        return this;
    }
    pipeline* set_viewport( vk::Viewport const & vp)
    {
        m_viewport = vp;
        return this;
    }

    pipeline* set_cull_mode(vk::CullModeFlags flags)
    {
        m_Rasterizer.cullMode = flags;
        return this;
    }

    pipeline* set_polygon_mode( vk::PolygonMode mode )
    {
        m_Rasterizer.polygonMode = mode;
        return this;
    }

    pipeline* set_front_face( vk::FrontFace face )
    {
        m_Rasterizer.frontFace = face;
        return this;
    }

    pipeline* set_vertex_shader( vka::shader * shader, std::string entry_point="main")
    {
        m_VertexShader = shader;
        m_VertexShaderEntry = entry_point;
        return this;
    }

    pipeline* set_fragment_shader( vka::shader * shader, std::string entry_point="main")
    {
        m_FragmentShader = shader;
        m_FragmentShaderEntry = entry_point;
        return this;
    }


    pipeline* set_render_pass( vka::renderpass * p);

    pipeline* set_vertex_attribute(uint32_t index, uint32_t offset, vk::Format format , uint32_t size);


    vk::PipelineLayout  get_layout() const {
        return m_PipelineLayout;
    }

    pipeline* add_texture_layout_binding(        uint32_t set, uint32_t binding, vk::ShaderStageFlags stages);
    pipeline* add_uniform_layout_binding(        uint32_t set, uint32_t binding, vk::ShaderStageFlags stages);
    pipeline* add_dynamic_uniform_layout_binding(uint32_t set, uint32_t binding, vk::ShaderStageFlags stages);
    pipeline* add_push_constant(uint32_t size, uint32_t offset, vk::ShaderStageFlags stages);




    /**
     * @brief create_new_descriptor_set
     * @param set - the set number to create the descriptor set out of
     * @param pool - the pool to allocate the descriptor set from
     * @return
     *
     * Create a new descriptor set based on the layout created in this pipeline.
     */
    descriptor_set* create_new_descriptor_set(uint32_t set, descriptor_pool * pool);

    void create();

    friend class context;
    friend class deleter<pipeline>;

};

}

#endif
