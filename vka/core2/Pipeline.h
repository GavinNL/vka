#ifndef VKA_CORE2_PIPELINE_H
#define VKA_CORE2_PIPELINE_H

#include <vulkan/vulkan.hpp>
#include <vka/core/context_child.h>

#include <vka/core2/Shader.h>
#include <vka/core2/DescriptorSet.h>

#include <map>

namespace vka
{

class DescriptorSet;
class DescriptorPool;

class  Pipeline : public context_child
{
public:
    vk::PipelineLayout      m_PipelineLayout;

    vk::Pipeline            m_pipeline;


    vk::Rect2D   m_scissor;
    vk::Viewport m_viewport;

    vk::PipelineDepthStencilStateCreateInfo  m_DepthStencil;

    vk::PipelineViewportStateCreateInfo      m_ViewportState;
    vk::PipelineRasterizationStateCreateInfo m_Rasterizer;

    vk::PipelineMultisampleStateCreateInfo   m_Multisampling;

    std::vector<vk::PipelineColorBlendAttachmentState> m_ColorBlendAttachments;

    vk::PipelineColorBlendStateCreateInfo    m_ColorBlending;

    vk::PipelineVertexInputStateCreateInfo   m_VertexInputInfo;
    vk::PipelineInputAssemblyStateCreateInfo m_InputAssembly;

    vk::PipelineTessellationStateCreateInfo  m_TesselationState;

    std::vector<vk::VertexInputAttributeDescription>  m_VertexAttributeDescription;

    std::vector<vk::VertexInputBindingDescription>    m_VertexBindDescriptions;

    std::vector<vk::PushConstantRange>                m_PushConstantRange;

    struct  DescriptorSetLayoutBindingInfo
    {
        vk::DescriptorSetLayoutCreateFlags flags;
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
    };
    //std::map<uint32_t , std::vector<vk::DescriptorSetLayoutBinding> >
    //                                                 m_DescriptorSetLayoutBindings;
    std::map<uint32_t , DescriptorSetLayoutBindingInfo >
                                                     m_DescriptorSetLayoutBindings;

    Shader_p        m_VertexShader;
    std::string   m_VertexShaderEntry;

    Shader_p        m_FragmentShader;
    std::string   m_FragmentShaderEntry;

    Shader_p        m_GeometryShader;
    std::string   m_GeometryShaderEntry;

    Shader_p        m_TesselationEvalShader;
    std::string   m_TesselationEvalShaderEntry;

    Shader_p        m_TesselationControlShader;
    std::string   m_TesselationControlShaderEntry;

    vk::RenderPass     m_RenderPass_raw;

    Pipeline(context * parent) :
        context_child(parent)
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




        //==========
        m_ColorBlending.logicOpEnable     = VK_FALSE;
        m_ColorBlending.logicOp           = vk::LogicOp::eCopy;
        m_ColorBlending.blendConstants[0] = 1.0f;
        m_ColorBlending.blendConstants[1] = 1.0f;
        m_ColorBlending.blendConstants[2] = 1.0f;
        m_ColorBlending.blendConstants[3] = 1.0f;
        m_ColorBlending.attachmentCount   = m_ColorBlendAttachments.size();
        m_ColorBlending.pAttachments      = m_ColorBlendAttachments.data();
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

    ~Pipeline();

public:

    operator vk::Pipeline()
    {
        return m_pipeline;
    }

    Pipeline* setScissor( vk::Rect2D const & sc)
    {
        m_scissor = sc;
        return this;
    }
    Pipeline* setViewport( vk::Viewport const & vp)
    {
        m_viewport = vp;
        return this;
    }

    Pipeline* setCullMode(vk::CullModeFlags flags)
    {
        m_Rasterizer.cullMode = flags;
        return this;
    }

    Pipeline* setPolygonMode( vk::PolygonMode mode )
    {
        m_Rasterizer.polygonMode = mode;
        return this;
    }

    Pipeline* setFrontFace( vk::FrontFace face )
    {
        m_Rasterizer.frontFace = face;
        return this;
    }

    Pipeline * setVertexShader  ( const std::string & path  , std::string const & entry_point);
    Pipeline * setFragmentShader( const std::string & path, std::string const & entry_point);
    Pipeline * setGeometryShader( const std::string & path, std::string const & entry_point);
    Pipeline * setTesselationControlShader( const std::string & path, std::string const & entry_point);
    Pipeline * setTesselationEvaluationShader( const std::string & path, std::string const & entry_point);

    Pipeline* setVertexShader( Shader_p shader, std::string entry_point="main")
    {
        m_VertexShader = shader;
        m_VertexShaderEntry = entry_point;
        return this;
    }

    Pipeline* setFragmentShader( Shader_p shader, std::string entry_point="main")
    {
        m_FragmentShader = shader;
        m_FragmentShaderEntry = entry_point;
        return this;
    }

    Pipeline* setGeometryShader( Shader_p shader, std::string entry_point="main")
    {
        m_GeometryShader = shader;
        m_GeometryShaderEntry = entry_point;
        return this;
    }

    Pipeline* setTesselationControlShader( Shader_p shader, std::string entry_point="main")
    {
        m_TesselationControlShader = shader;
        m_TesselationControlShaderEntry = entry_point;
        return this;
    }

    Pipeline* setTesselationEvaluationShader( Shader_p shader, std::string entry_point="main")
    {
        m_TesselationEvalShader = shader;
        m_TesselationEvalShaderEntry = entry_point;
        return this;
    }

    Pipeline* setTopology( vk::PrimitiveTopology r )
    {
        m_InputAssembly.topology = r;
        return this;
    }

    Pipeline* setTesselationPatchControlPoints(uint32_t num_points)
    {
        m_TesselationState.patchControlPoints = num_points;
        return this;
    }

    Pipeline* setLineWidth(float f)
    {
        m_Rasterizer.lineWidth = f;
        return this;
    }

    Pipeline* setRenderPass( vk::RenderPass P);

    Pipeline* setVertexAttribute(uint32_t binding, uint32_t location, uint32_t offset, vk::Format format , uint32_t size);


    vk::PipelineLayout  getLayout() const {
        return m_PipelineLayout;
    }

    Pipeline* addTextureLayoutBinding(        uint32_t set, uint32_t binding, vk::ShaderStageFlags stages);
    Pipeline* addUniformLayoutBinding(        uint32_t set, uint32_t binding, vk::ShaderStageFlags stages);
    Pipeline* addDynamicUniformLayoutBinding(uint32_t set, uint32_t binding, vk::ShaderStageFlags stages);
    Pipeline* addPushConstant(uint32_t size, uint32_t offset, vk::ShaderStageFlags stages);

    Pipeline* enablePushDescriptor(uint32_t set_number)
    {
        m_DescriptorSetLayoutBindings[set_number].flags |= vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR;
        return this;
    }


    Pipeline* addColorBlendAttachmentState( const vk::PipelineColorBlendAttachmentState & C)
    {
        m_ColorBlendAttachments.push_back(C);
        return this;
    }

    vk::PipelineColorBlendAttachmentState& getColorBlendAttachmentState(uint32_t i)
    {
        return m_ColorBlendAttachments[i];
    }

    Pipeline* setColorAttachments(uint32_t num)
    {
        vk::PipelineColorBlendAttachmentState C;
        C.colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        C.blendEnable         = VK_TRUE;
        C.colorBlendOp        = vk::BlendOp::eAdd;
        C.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
        C.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        C.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusDstAlpha;
        C.dstColorBlendFactor = vk::BlendFactor::eOneMinusDstAlpha;

        for(uint32_t i =0;i<num;++i)
            addColorBlendAttachmentState(C);

        return this;
    }

    /**
     * @brief create_new_descriptor_set
     * @param set - the set number to create the descriptor set out of
     * @param pool - the pool to allocate the descriptor set from
     * @return
     *
     * Create a new descriptor set based on the layout created in this Pipeline.
     */
    vka::DescriptorSet_p createNewDescriptorSet(uint32_t set, DescriptorPool * pool);

    void create();

    friend class context;
    friend class deleter<Pipeline>;

};

}

#endif
