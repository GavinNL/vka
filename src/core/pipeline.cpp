#include <vka/core/log.h>
#include <vka/core/pipeline.h>
#include <vka/core/context.h>
#include <vka/core/shader.h>
#include <vka/core/renderpass.h>
#include <vka/core/descriptor_pool.h>
#include <vka/core/descriptor_set.h>

vka::pipeline::~pipeline()
{
    if(m_PipelineLayout)
        get_device().destroyPipelineLayout( m_PipelineLayout);

    if( m_pipeline )
    {
        get_device().destroyPipeline( m_pipeline );
    }
}

vka::pipeline* vka::pipeline::set_vertex_attribute(uint32_t binding, uint32_t location, uint32_t offset, vk::Format format , uint32_t stride)
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


vka::pipeline *vka::pipeline::add_texture_layout_binding(uint32_t set, uint32_t binding, vk::ShaderStageFlags stages)
{
    vk::DescriptorSetLayoutBinding samplerLayoutBinding;
    samplerLayoutBinding.binding            = binding;
    samplerLayoutBinding.descriptorCount    = 1;
    samplerLayoutBinding.descriptorType     = vk::DescriptorType::eCombinedImageSampler;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags         = stages;// VK_SHADER_STAGE_VERTEX_BIT;

    m_DescriptorSetLayoutBindings[set].push_back(samplerLayoutBinding);

    return this;
}

vka::pipeline *vka::pipeline::add_uniform_layout_binding(uint32_t set, uint32_t binding, vk::ShaderStageFlags stages)
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding;
    uboLayoutBinding.binding            = binding;
    uboLayoutBinding.descriptorCount    = 1;
    uboLayoutBinding.descriptorType     = vk::DescriptorType::eUniformBuffer;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags         = stages;// VK_SHADER_STAGE_VERTEX_BIT;

    m_DescriptorSetLayoutBindings[set].push_back(uboLayoutBinding);

    return this;
}

vka::pipeline *vka::pipeline::add_dynamic_uniform_layout_binding(uint32_t set, uint32_t binding, vk::ShaderStageFlags stages)
{
    vk::DescriptorSetLayoutBinding duboLayoutBinding;

    duboLayoutBinding.binding            = binding;
    duboLayoutBinding.descriptorCount    = 1;
    duboLayoutBinding.descriptorType     = vk::DescriptorType::eUniformBufferDynamic;
    duboLayoutBinding.pImmutableSamplers = nullptr;
    duboLayoutBinding.stageFlags         = stages;// VK_SHADER_STAGE_VERTEX_BIT;

    m_DescriptorSetLayoutBindings[set].push_back(duboLayoutBinding);

    return this;
}

vka::pipeline * vka::pipeline::add_push_constant(uint32_t size, uint32_t offset, vk::ShaderStageFlags stages)
{
    m_PushConstantRange.push_back( vk::PushConstantRange(stages, offset, size) );
    return this;
}

vka::descriptor_set* vka::pipeline::create_new_descriptor_set(uint32_t set, descriptor_pool * pool)
{
    descriptor_set * S = pool->allocate_descriptor_set();
    S->create( m_DescriptorSetLayoutBindings.at(set) );
    return S;
}

vka::pipeline* vka::pipeline::set_render_pass( vka::renderpass * p)
{
    m_RenderPass = p;
    return this;
}

vka::pipeline * vka::pipeline::set_vertex_shader( const std::string & path  , std::string const & entry_point)
{
    static int i=0;
    auto * p = get_parent_context()->new_shader_module( std::string("vertex_shader_module_" + std::to_string(i++) ) );
    p->load_from_file(path);
    set_vertex_shader(p, entry_point);
    return this;
}

vka::pipeline * vka::pipeline::set_fragment_shader( const std::string & path  , std::string const & entry_point)
{
    static int i=0;
    auto * p = get_parent_context()->new_shader_module( std::string("fragment_shader_module_" + std::to_string(i++) ) );
    p->load_from_file(path);
    set_fragment_shader(p, entry_point);
    return this;
}


void vka::pipeline::create()
{
    auto device = get_device(); //Device::GetGlobal().m_Device;

//    if( !I.m_RenderPass )
//    {
//        throw std::runtime_error("Renderpass not set. Make sure to use Pipeline::SetRenderPass( )");
//    }
//
//    if( !I.m_RenderPass )
//    {
//        throw std::runtime_error("Renderpass not created. Make sure you run renderpass->create() first");
//    }
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

        add_color_blend_attachment_state(C);
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


    // Attach the descriptor sets
    //

   // vk::DescriptorSetLayoutCreateInfo C;
   // C.bindingCount = I.m_DescriptorSetLayoutBindings.size();
   // C.pBindings    = I.m_DescriptorSetLayoutBindings.data();

    //I.m_DescriptorSetLayouts.push_back(  device.createDescriptorSetLayout(C) );
    //
    //if (!I.m_DescriptorSetLayouts[0])
    //{
    //    throw std::runtime_error("Failed to create descriptor set layout!");
    //}
    //LOG << "Descriptor Set Layouts created" << END;



    std::vector<vk::DescriptorSetLayout> layouts;
    for(auto & bindings : m_DescriptorSetLayoutBindings)
    {
        layouts.push_back(  get_parent_context()->new_descriptor_set_layout( bindings.second )->get() );
    }

   // for(auto  d : m_DescriptorSetLayouts)
   // {
   //     layouts.push_back( *d );
   // }


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


    pipelineInfo.renderPass          = m_RenderPass->m_RenderPass;
    pipelineInfo.subpass = 0;


    m_pipeline = device.createGraphicsPipeline(vk::PipelineCache(),  pipelineInfo);


    if( !m_pipeline )
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
    LOG << "Pipeline created" << ENDL;
}
