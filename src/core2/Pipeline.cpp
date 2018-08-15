#include <vka/core/log.h>
#include <vka/core/context.h>
#include <vka/core2/DescriptorPool.h>
#include <vka/core/descriptor_set_layout.h>
#include <vka/core2/DescriptorSet.h>


#include <vka/core2/Pipeline.h>
#include <vka/core2/Shader.h>

namespace vka
{

Pipeline::~Pipeline()
{
    if(m_PipelineLayout)
        get_device().destroyPipelineLayout( m_PipelineLayout);

    if( m_pipeline )
    {
        get_device().destroyPipeline( m_pipeline );
    }
}

vka::Pipeline* Pipeline::setVertexAttribute(uint32_t binding, uint32_t location, uint32_t offset, vk::Format format , uint32_t stride)
{
    auto & ad = m_VertexAttributeDescription;
    vk::VertexInputAttributeDescription AD;

    AD.binding  = binding;
    AD.location = location;
    AD.format   = format;
    AD.offset   = offset;

    ad.push_back(AD);

  //  uint32_t s = size;

    LOG  << "Adding vertex attribute: "  <<
            "index: " << location  << ", " <<
            "offset:" << offset << ", " <<
            "format:" << vk::to_string(format) << ENDL;



    auto it = std::find_if(m_VertexBindDescriptions.begin(),
                           m_VertexBindDescriptions.end(),
                           [=](const vk::VertexInputBindingDescription& obj)
                           {
                               return obj.binding == binding;
                           });
    if( it == m_VertexBindDescriptions.end())
    {
        vk::VertexInputBindingDescription V;
        V.binding   = binding;
        V.stride    = stride;
        V.inputRate = vk::VertexInputRate::eVertex;
        m_VertexBindDescriptions.push_back(V);
    }

    m_VertexInputInfo.vertexBindingDescriptionCount   = m_VertexBindDescriptions.size();
    m_VertexInputInfo.pVertexBindingDescriptions      = m_VertexBindDescriptions.data();

    m_VertexInputInfo.vertexAttributeDescriptionCount = m_VertexAttributeDescription.size();
    m_VertexInputInfo.pVertexAttributeDescriptions    = m_VertexAttributeDescription.data();

    return this;
}


vka::Pipeline *Pipeline::addTextureLayoutBinding(uint32_t set, uint32_t binding, vk::ShaderStageFlags stages)
{
    vk::DescriptorSetLayoutBinding samplerLayoutBinding;
    samplerLayoutBinding.binding            = binding;
    samplerLayoutBinding.descriptorCount    = 1;
    samplerLayoutBinding.descriptorType     = vk::DescriptorType::eCombinedImageSampler;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags         = stages;// VK_SHADER_STAGE_VERTEX_BIT;

    m_DescriptorSetLayoutBindings[set].bindings.push_back(samplerLayoutBinding);

    return this;
}

vka::Pipeline *Pipeline::addUniformLayoutBinding(uint32_t set, uint32_t binding, vk::ShaderStageFlags stages)
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding;
    uboLayoutBinding.binding            = binding;
    uboLayoutBinding.descriptorCount    = 1;
    uboLayoutBinding.descriptorType     = vk::DescriptorType::eUniformBuffer;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags         = stages;// VK_SHADER_STAGE_VERTEX_BIT;

    m_DescriptorSetLayoutBindings[set].bindings.push_back(uboLayoutBinding);

    return this;
}

vka::Pipeline *Pipeline::addDynamicUniformLayoutBinding(uint32_t set, uint32_t binding, vk::ShaderStageFlags stages)
{
    vk::DescriptorSetLayoutBinding duboLayoutBinding;

    duboLayoutBinding.binding            = binding;
    duboLayoutBinding.descriptorCount    = 1;
    duboLayoutBinding.descriptorType     = vk::DescriptorType::eUniformBufferDynamic;
    duboLayoutBinding.pImmutableSamplers = nullptr;
    duboLayoutBinding.stageFlags         = stages;// VK_SHADER_STAGE_VERTEX_BIT;

    m_DescriptorSetLayoutBindings[set].bindings.push_back(duboLayoutBinding);

    return this;
}

vka::Pipeline * Pipeline::addPushConstant(uint32_t size, uint32_t offset, vk::ShaderStageFlags stages)
{
    m_PushConstantRange.push_back( vk::PushConstantRange(stages, offset, size) );
    return this;
}

vka::DescriptorSet_p Pipeline::createNewDescriptorSet(uint32_t set, DescriptorPool * pool)
{
    auto S = pool->allocateDescriptorSet();
    S->create( m_DescriptorSetLayoutBindings.at(set).bindings );
    return S;
}

vka::Pipeline* Pipeline::setRenderPass(vk::RenderPass P)
{
    m_RenderPass_raw = P;
    return this;
}

vka::Pipeline * Pipeline::setVertexShader( const std::string & path  , std::string const & entry_point)
{
    static int i=0;

    auto p = std::make_shared<Shader>(get_parent_context());

    p->loadFromFile(path);
    setVertexShader(p, entry_point);
    return this;
}

vka::Pipeline * Pipeline::setFragmentShader( const std::string & path  , std::string const & entry_point)
{
    static int i=0;
    auto p = std::make_shared<Shader>(get_parent_context());

    p->loadFromFile(path);
    setFragmentShader(p, entry_point);
    return this;
}

vka::Pipeline * Pipeline::setGeometryShader( const std::string & path  , std::string const & entry_point)
{
    static int i=0;
    auto p = std::make_shared<Shader>(get_parent_context());

    p->loadFromFile(path);
    setGeometryShader(p, entry_point);
    return this;
}

vka::Pipeline * Pipeline::setTesselationControlShader( const std::string & path  , std::string const & entry_point)
{
    static int i=0;
    auto p = std::make_shared<Shader>(get_parent_context());

    p->loadFromFile(path);
    setTesselationControlShader(p, entry_point);
    return this;
}

vka::Pipeline * Pipeline::setTesselationEvaluationShader( const std::string & path  , std::string const & entry_point)
{
    static int i=0;
    auto p = std::make_shared<Shader>(get_parent_context());

    p->loadFromFile(path);
    setTesselationEvaluationShader(p, entry_point);
    return this;
}

void Pipeline::create()
{
    auto device = get_device(); //Device::GetGlobal().m_Device;

    if( m_ColorBlendAttachments.size() == 0)
    {
        vk::PipelineColorBlendAttachmentState C;
        C.colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        C.blendEnable         = VK_TRUE;
        C.colorBlendOp        = vk::BlendOp::eAdd;
        C.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
        C.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        C.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusDstAlpha;
        C.dstColorBlendFactor = vk::BlendFactor::eOneMinusDstAlpha;

        addColorBlendAttachmentState(C);
    }
    m_ColorBlending.attachmentCount   = m_ColorBlendAttachments.size();
    m_ColorBlending.pAttachments      = m_ColorBlendAttachments.data();

    // setup the viewport
    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &m_viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &m_scissor;


    //=================================
    // Create the pipeline layout

    std::vector<vk::DescriptorSetLayout> layouts;
    for(auto & bindings : m_DescriptorSetLayoutBindings)
    {
        layouts.push_back(  get_parent_context()->new_descriptor_set_layout( bindings.second.bindings , bindings.second.flags)->get() );
    }

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        pipelineLayoutInfo.setLayoutCount     = static_cast<uint32_t>(layouts.size());
    pipelineLayoutInfo.pSetLayouts            = layouts.data();

    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>( m_PushConstantRange.size() );
    pipelineLayoutInfo.pPushConstantRanges    = m_PushConstantRange.data();


    m_PipelineLayout = device.createPipelineLayout( pipelineLayoutInfo );

    if( !m_PipelineLayout)
    {
        throw std::runtime_error("Error creating pipeline layout");
    }
    LOG << "Pipeline layout created" << ENDL;

    //==== Setup the shader stages
    std::vector<vk::PipelineShaderStageCreateInfo> shaderstages;

    if( m_VertexShader )
    {
        vk::PipelineShaderStageCreateInfo st;
        st.stage  = vk::ShaderStageFlagBits::eVertex;
        st.module = m_VertexShader->m_shader;
        st.pName  = m_VertexShaderEntry.data();// "main";

        shaderstages.push_back( st );
        LOG << "  Adding Vertex Shader. Entry point:  " << m_VertexShaderEntry << ENDL;
    }
    if( m_FragmentShader )
    {
        vk::PipelineShaderStageCreateInfo st;
        st.stage  = vk::ShaderStageFlagBits::eFragment;
        st.module = m_FragmentShader->m_shader;
        st.pName  = m_FragmentShaderEntry.data();//"main";
        shaderstages.push_back( st );
        LOG << "  Adding Fragment Shader. Entry point:  " << m_FragmentShaderEntry<< ENDL;
    }
    if( m_TesselationControlShader )
    {
        vk::PipelineShaderStageCreateInfo st;
        st.stage  = vk::ShaderStageFlagBits::eTessellationControl;
        st.module = m_TesselationControlShader->m_shader;
        st.pName  = m_TesselationControlShaderEntry.data();//"main";
        shaderstages.push_back( st );
        LOG << "  Adding Tesellation Control Shader. Entry point:  " << m_TesselationControlShaderEntry << ENDL;
    }
    if( m_TesselationEvalShader)
    {
        vk::PipelineShaderStageCreateInfo st;
        st.stage  = vk::ShaderStageFlagBits::eTessellationEvaluation;
        st.module = m_TesselationEvalShader->m_shader;
        st.pName  = m_TesselationEvalShaderEntry.data();//"main";
        shaderstages.push_back( st );
        LOG << "  Adding Tesellation Evaluation Shader. Entry point:  " << m_TesselationEvalShaderEntry << ENDL;
    }
    //===========================================
    //===========================================


    // At last create the pipeline
    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.stageCount          = shaderstages.size();
    pipelineInfo.pStages             = shaderstages.data();
    pipelineInfo.pVertexInputState   = &m_VertexInputInfo;
    pipelineInfo.pInputAssemblyState = &m_InputAssembly;
    pipelineInfo.pViewportState      = &m_ViewportState;
    pipelineInfo.pRasterizationState = &m_Rasterizer;
    pipelineInfo.pMultisampleState   = &m_Multisampling;

    pipelineInfo.pDepthStencilState  = &m_DepthStencil;
    pipelineInfo.pColorBlendState    = &m_ColorBlending;

    pipelineInfo.layout              = m_PipelineLayout;

    if( m_TesselationState.patchControlPoints!=0)
    {
        pipelineInfo.pTessellationState  = &m_TesselationState;
    }

    pipelineInfo.renderPass          = m_RenderPass_raw;

    pipelineInfo.subpass = 0;


    m_pipeline = device.createGraphicsPipeline(vk::PipelineCache(),  pipelineInfo);


    if( !m_pipeline )
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
    LOG << "Pipeline created" << ENDL;
}

}
