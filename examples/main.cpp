#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <vka/core/image.h>
#include <vka/vka.h>


#include <glm/glm.hpp>

#define WIDTH 1024
#define HEIGHT 768
#define APP_TITLE "Test"

int main(int argc, char ** argv)
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE,  GLFW_FALSE);

    auto window = glfwCreateWindow(WIDTH, HEIGHT, APP_TITLE, nullptr, nullptr);


    vka::context C;

    C.init();
    C.create_window_surface(window);
    C.create_device();

    C.create_swap_chain( {WIDTH,HEIGHT});

    auto R = C.new_renderpass("main_renderpass");
    R->attach_color(vk::Format::eB8G8R8A8Unorm);
    R->create(C);


    auto cp = C.new_command_pool("main_command_pool");

    std::vector<vka::framebuffer*> framebuffers;
    auto & iv = C.get_swapchain_imageviews();
    int i=0;
    for(auto & view : iv)
    {
        framebuffers.push_back(  C.new_framebuffer( std::string("fb_") + std::to_string(i++) ) );
        framebuffers.back()->create( *R, vk::Extent2D{WIDTH,HEIGHT}, view, vk::ImageView());
    }


    //====================================
    auto descriptor_pool = C.new_descriptor_pool("main_desc_pool");
    descriptor_pool->set_pool_size(vk::DescriptorType::eCombinedImageSampler, 2);
    descriptor_pool->create();
    //====================================



    auto * vertex_shader = C.new_shader_module("vs");
    vertex_shader->load_from_file("../resources/shaders/hello_textured_triangle/hello_textured_triangle_v.spv");

    auto * fragment_shader = C.new_shader_module("fs");
    fragment_shader->load_from_file("../resources/shaders/hello_textured_triangle/hello_textured_triangle_f.spv");

    auto * pipeline = C.new_pipeline("triangle");

    // Create the graphics Pipeline
      pipeline->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
              ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

              ->set_vertex_shader(   vertex_shader ) // the shaders we want to use
              ->set_fragment_shader( fragment_shader ) // the shaders we want to use

              // tell the pipeline that attribute 0 contains 3 floats
              // and the data starts at offset 0
              ->set_vertex_attribute(0 ,  0,  vk::Format::eR32G32B32Sfloat,  sizeof(glm::vec3)+sizeof(glm::vec2) )
              // tell the pipeline that attribute 1 contains 3 floats
              // and the data starts at offset 12
              ->set_vertex_attribute(1 , 12,  vk::Format::eR32G32Sfloat,  sizeof(glm::vec3)+sizeof(glm::vec2) )

              // Don't cull the
              ->set_cull_mode(vk::CullModeFlagBits::eNone)
              ->set_front_face(vk::FrontFace::eCounterClockwise)
              ->add_texture_layout_binding(0, 0, vk::ShaderStageFlagBits::eFragment)
              //
              ->set_render_pass( R )
              ->create();

    auto * set = pipeline->create_new_descriptor_set(0, descriptor_pool);


#define USE_STAGING

#if !defined USE_STAGING
      auto * vb = C.new_buffer("vb", 1024, vk::MemoryPropertyFlagBits::eHostVisible| vk::MemoryPropertyFlagBits::eHostCoherent, vk::BufferUsageFlagBits::eVertexBuffer );
      auto * ib = C.new_buffer("ib", 1024, vk::MemoryPropertyFlagBits::eHostVisible| vk::MemoryPropertyFlagBits::eHostCoherent, vk::BufferUsageFlagBits::eIndexBuffer);

      if(1)
      {


          auto vertex =  vb->map<glm::vec3>();
          vertex[0] = glm::vec3(0, -1.0, 0.0);
          vertex[1] = glm::vec3(1, 0   ,0);

          vertex[2] = glm::vec3(-1.0, 0.0,0.0);
          vertex[3] = glm::vec3(0,1,0);

          vertex[4] = glm::vec3( 1.0, 1.0   ,0.0);
          vertex[5] = glm::vec3(0,1,0);

          auto index =  ib->map<glm::uint16>();
          index[0] = 0;
          index[1] = 1;
          index[2] = 2;

          vb->unmap_memory();
          ib->unmap_memory();
      }
#else

    auto * vb = C.new_vertex_buffer("vb", 1024 );
    auto * ib = C.new_index_buffer( "ib", 1024 );
    auto * sb = C.new_staging_buffer( "sb", 1024 );


    if(1)
    {
        struct Vertex
        {
            glm::vec3 p;
            glm::vec2 u;
        };

        auto vertex =  sb->map<Vertex>();

        LOG << "Vertex size: " << vertex.size() << ENDL;
        vertex[0] = {glm::vec3(0, -1.0, 0.0)     , glm::vec2(0.5 , 0) } ;
        vertex[1] = {glm::vec3(-1.0, 0.0,0.0)    , glm::vec2(0   , 1) };
        vertex[2] = {glm::vec3( 1.0, 1.0   ,0.0) , glm::vec2(1   , 1) };


        auto index =  sb->map<glm::uint16>( 3*sizeof(Vertex));
        index[0] = 0;
        index[1] = 1;
        index[2] = 2;

        LOG << "Index size: " << index.size() << ENDL;
        ////===============
        auto copy_cmd = cp->AllocateCommandBuffer();
        copy_cmd.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );

        copy_cmd.copyBuffer( *sb , *vb, vk::BufferCopy{ 0,0,3*sizeof(Vertex)} );
        copy_cmd.copyBuffer( *sb , *ib, vk::BufferCopy{ 3*sizeof(Vertex),0,3*sizeof(uint16_t) } );

        copy_cmd.end();
        C.submit_cmd_buffer(copy_cmd);
        ////===============
        //
        cp->FreeCommandBuffer(copy_cmd);

        vb->unmap_memory();
        ib->unmap_memory();
    }
#endif


    // C.new_texture_2d("test_texture", w,h,d, vk::Format::eR8G8B8A8Unorm);
    // C.new_texture_2d("test_texture", w,h,d, vk::Format::eR8G8B8A8Unorm);


        vka::image D;
        D.load_from_path("../resources/textures/Brick-2852a.jpg",4);

    auto * staging_texture = C.new_texture("staging_texture");
    staging_texture->set_size(512,512,1);
    staging_texture->set_tiling(vk::ImageTiling::eLinear);
    staging_texture->set_usage(  vk::ImageUsageFlagBits::eSampled  | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc );
    staging_texture->set_memory_properties( vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    staging_texture->set_format(vk::Format::eR8G8B8A8Unorm);
    staging_texture->set_view_type(vk::ImageViewType::e2D);
    staging_texture->set_mipmap_levels(1);
    staging_texture->create();
    staging_texture->create_image_view(vk::ImageAspectFlagBits::eColor);

    void * image_data = staging_texture->map_memory();
    LOG << "Image size: " << D.size() << ENDL;
    memcpy(image_data, D.data(), D.size() );


    auto * tex = C.new_texture("test_texture");
    tex->set_size(512,512,1);
    tex->set_tiling(vk::ImageTiling::eOptimal);
    tex->set_usage(  vk::ImageUsageFlagBits::eSampled  | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc );
    tex->set_memory_properties( vk::MemoryPropertyFlagBits::eDeviceLocal);
    tex->set_format(vk::Format::eR8G8B8A8Unorm);
    tex->set_view_type(vk::ImageViewType::e2D);
    tex->set_mipmap_levels(1);
    tex->create();
    tex->create_image_view(vk::ImageAspectFlagBits::eColor);
    tex->create_sampler();





    // Convert the staging texture into a TransferSrcOptimal so that it can be
    // transferred to the device texture
    {
        auto cb1 = cp->AllocateCommandBuffer();
        cb1.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );
        staging_texture->convert_layer(cb1, vk::ImageLayout::eGeneral,0,0);
        cb1.end();
        C.submit_cmd_buffer(cb1);
        cp->FreeCommandBuffer(cb1);
    }
    {
        auto cb1 = cp->AllocateCommandBuffer();
        cb1.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );
         staging_texture->convert_layer(cb1, vk::ImageLayout::eTransferSrcOptimal,0,0);
        cb1.end();
        C.submit_cmd_buffer(cb1);
        cp->FreeCommandBuffer(cb1);
    }
    {
        auto cb1 = cp->AllocateCommandBuffer();
        cb1.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );
         tex->convert_layer(cb1, vk::ImageLayout::eTransferDstOptimal,0,0);
        cb1.end();
        C.submit_cmd_buffer(cb1);
        cp->FreeCommandBuffer(cb1);
    }
    {
        auto cb1 = cp->AllocateCommandBuffer();
        cb1.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );

        vk::ImageCopy IC;
        vk::ImageSubresourceLayers subResource;
        subResource.aspectMask     = vk::ImageAspectFlagBits::eColor;//  VK_IMAGE_ASPECT_COLOR_BIT;
        subResource.baseArrayLayer = 0;
        subResource.mipLevel       = 0;
        subResource.layerCount     = 1;
        IC.setDstOffset( vk::Offset3D(0,0,0))
          .setSrcOffset(vk::Offset3D(0,0,0))
          .setExtent(vk::Extent3D(512,512,1))
          .setSrcSubresource(subResource)
          .setDstSubresource(subResource);

        cb1.copyImage( staging_texture->get_image(),
                       vk::ImageLayout::eTransferSrcOptimal ,
                       tex->get_image(),
                       vk::ImageLayout::eTransferDstOptimal,
                       IC);
        cb1.end();
        C.submit_cmd_buffer(cb1);
        cp->FreeCommandBuffer(cb1);
    }
    {
        auto cb1 = cp->AllocateCommandBuffer();
        cb1.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );
         tex->convert_layer(cb1, vk::ImageLayout::eShaderReadOnlyOptimal,0,0);
        cb1.end();
        C.submit_cmd_buffer(cb1);
        cp->FreeCommandBuffer(cb1);
    }
#if 0
        auto cb1 = cp->AllocateCommandBuffer();
        cb1.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );

        staging_texture->convert_layer(cb1, vk::ImageLayout::eGeneral,0,0);


        staging_texture->convert_layer(cb1, vk::ImageLayout::eTransferSrcOptimal,0,0);
        tex->convert_layer(cb1, vk::ImageLayout::eTransferDstOptimal,0,0);

        // need to implement this:
        //  Records the following into the command buffer:
        //    - Convert staging-texture into eTransferSrcOptimal
        //    - Convert tex into eTransferDstOptimal
        //    - Converts
        //tex->copy_image( cb1, staging_texture, vk::Offset3D(0,0), vk::Extent3D(1024,1024,1) );
        //tex->copy_buffer(cb1, staging_buffer,)

        //======================================================================
        vk::ImageCopy IC;
        vk::ImageSubresourceLayers subResource;
        subResource.aspectMask     = vk::ImageAspectFlagBits::eColor;//  VK_IMAGE_ASPECT_COLOR_BIT;
        subResource.baseArrayLayer = 0;
        subResource.mipLevel       = 0;
        subResource.layerCount     = 1;
        IC.setDstOffset( vk::Offset3D(0,0,0))
          .setSrcOffset(vk::Offset3D(0,0,0))
          .setExtent(vk::Extent3D(512,512,1))
          .setSrcSubresource(subResource)
          .setDstSubresource(subResource);

        cb1.copyImage( staging_texture->get_image(),
                       vk::ImageLayout::eTransferSrcOptimal ,
                       tex->get_image(),
                       vk::ImageLayout::eTransferDstOptimal,
                       IC);

        tex->convert_layer(cb1, vk::ImageLayout::eShaderReadOnlyOptimal,0,0);

        cb1.end();
        C.submit_cmd_buffer(cb1);
        cp->FreeCommandBuffer(cb1);

        set->attach_sampler(0, tex);
        set->update();
#endif


    set->attach_sampler(0, tex);
    set->update();
    //tex->convert( vk::ImageLayout::eTransferSrcOptimal);



    auto cb = cp->AllocateCommandBuffer();


    auto * image_available_semaphore = C.new_semaphore("image_available_semaphore");
    auto * render_complete_semaphore = C.new_semaphore("render_complete_semaphore");

    uint32_t fb_index=0;
    while (!glfwWindowShouldClose(window) )
    {
      static bool once = true;

      glfwPollEvents();

      if(once)
      {
          //once = false;

      fb_index = C.get_next_image_index(image_available_semaphore);

      cb.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
      //  BEGIN RENDER PASS=====================================================
      cb.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse) );

      // We want the to use the render pass we created earlier
      vk::RenderPassBeginInfo renderPassInfo;
      renderPassInfo.renderPass        = *R;
      renderPassInfo.framebuffer       = *framebuffers[fb_index];// m_Screen.get().m_Framebuffers[i];//  draw to the i'th frame buffer
      renderPassInfo.renderArea.offset = vk::Offset2D{0,0};

      renderPassInfo.renderArea.extent = vk::Extent2D(WIDTH,HEIGHT);// vka::Context_t::Get()->GetSwapChain()->extent();

      // Clear values are used to clear the various frame buffers
      // we want to clear the colour values to black
      // at the start of rendering.
      std::vector<vk::ClearValue> clearValues;
      clearValues.push_back( vk::ClearValue( vk::ClearColorValue( std::array<float,4>{0.0f, 0.f, 0.f, 1.f} ) ) );

      renderPassInfo.clearValueCount = clearValues.size();
      renderPassInfo.pClearValues    = clearValues.data();

      cb.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
      //  BEGIN RENDER PASS=====================================================


      auto dsp = vk::ArrayProxy<const vk::DescriptorSet>( set->get());

            cb.bindPipeline( vk::PipelineBindPoint::eGraphics, *pipeline );
            cb.bindDescriptorSets( vk::PipelineBindPoint::eGraphics,
                                                    pipeline->get_layout(),
                                                    0,
                                                    dsp,
                                                    nullptr );
            cb.bindVertexBuffers(0, vb->get(), {0} );// ( m_VertexBuffer, 0);
            cb.bindIndexBuffer(  ib->get() , 0 , vk::IndexType::eUint16);
            cb.drawIndexed(3, 1, 0 , 0, 0);

      cb.endRenderPass();
      cb.end();

      C.submit_command_buffer(cb, image_available_semaphore, render_complete_semaphore);

      C.present_image(fb_index, render_complete_semaphore);
        }


      std::this_thread::sleep_for( std::chrono::milliseconds(3) );
    }

    return 0;
}

#define STB_IMAGE_IMPLEMENTATION
#include<stb/stb_image.h>
