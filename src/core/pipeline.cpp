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

vka::pipeline* vka::pipeline::set_vertex_attribute(uint32_t index, uint32_t offset, vk::Format format , uint32_t stride)
{
    auto & ad = m_VertexAttributeDescription;
    //decltype(get().m_VertexAttributeDescription)::value_type AD;

    vk::VertexInputAttributeDescription AD;

    AD.binding  = 0;
    AD.location = index;
    AD.format   = format;
    AD.offset   = offset;

    ad.push_back(AD);

  //  uint32_t s = size;

    LOG  << "Adding vertex attribute: "  <<
            "index: " << index  << ", " <<
            "offset:" << offset << ", " <<
            "format:" << vk::to_string(format) << ENDL;



    m_VertexBindDescription.binding   = 0;
    m_VertexBindDescription.stride    = stride;
    m_VertexBindDescription.inputRate = vk::VertexInputRate::eVertex;

    m_VertexInputInfo.vertexBindingDescriptionCount   = 1;
    m_VertexInputInfo.pVertexBindingDescriptions      = &m_VertexBindDescription;

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
