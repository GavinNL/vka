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



    auto * vertex_buffer = C.new_vertex_buffer("vb", 1024 );
    auto * index_buffer  = C.new_index_buffer( "ib", 1024 );


//==============================================================================
// Create the Vertex and Index buffers
//
//  We are going to  create a vertex and index buffer. The vertex buffer will
//  hold the positions and UV coordates of our triangle.
//
//  The steps to create the buffer are.
//    1. Copy the vertex/index data from the host to a memory mappable device buffer
//    2. copy the memory-mapped buffer to the vertex/index buffers
//==============================================================================

        // This is the vertex structure we are going to use
        // it contains a position and a UV coordates field
        struct Vertex
        {
            glm::vec3 p;
            glm::vec2 u;
        };

        // allocate a staging buffer of 10MB
        auto * staging_buffer = C.new_staging_buffer( "sb", 1024*1024*10 );

        // using the map< > method, we can return an array_view into the
        // memory. We are going to place them in their own scope so that
        // the array_view is destroyed after exiting the scope. This is
        // so we do not accidenty access teh array_view after the
        // staging_buffer has been unmapped.
        {
            vka::array_view<Vertex> vertex =  staging_buffer->map<Vertex>();

            LOG << "Vertex size: " << vertex.size() << ENDL;
            // we can access each vertex as if it was an array. Copy the
            // vertex data we want into the first three indices.
            vertex[0] = {glm::vec3(0, -1.0, 0.0)     , glm::vec2(0.5 , 0) } ;
            vertex[1] = {glm::vec3(-1.0, 0.0,0.0)    , glm::vec2(0   , 1) };
            vertex[2] = {glm::vec3( 1.0, 1.0   ,0.0) , glm::vec2(1   , 1) };
        }
        // Do the same for the index buffer. but we want to specific an
        // offset form the start of the buffer so we do not overwrite the
        // vertex data.
        {
            vka::array_view<glm::uint16_t> index =  staging_buffer->map<glm::uint16>( 3*sizeof(Vertex));
            index[0] = 0;
            index[1] = 1;
            index[2] = 2;
            LOG << "Index size: " << index.size() << ENDL;
        }


        // 2. Copy the data from the host-visible buffer to the vertex/index buffers

        // allocate a comand buffer
        auto copy_cmd = cp->AllocateCommandBuffer();
        copy_cmd.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );

        // write the commands to copy each of the buffer data
        copy_cmd.copyBuffer( *staging_buffer , *vertex_buffer, vk::BufferCopy{ 0               ,0,3*sizeof(Vertex)} );
        copy_cmd.copyBuffer( *staging_buffer , *index_buffer , vk::BufferCopy{ 3*sizeof(Vertex),0,3*sizeof(uint16_t) } );

        copy_cmd.end();
        C.submit_cmd_buffer(copy_cmd);
        ////===============
        //
        cp->FreeCommandBuffer(copy_cmd);

        // Unmap the memory.
        //   WARNING: DO NOT ACCESS the vertex and index array_views as these
        //            now point to unknown memory spaces
        staging_buffer->unmap_memory();


//==============================================================================
// Create a texture
//
//==============================================================================

    // 1. First load host_image into memory.
        vka::host_image D("../resources/textures/Brick-2852a.jpg",4);
        //D.load_from_path


    // 2. Use the context's helper function to create a device local texture
    //    We will be using a texture2d which is a case specific version of the
    //    generic texture
        vka::texture2d * tex = C.new_texture2d("test_texture");
        tex->set_size( D.width() , D.height() );
        tex->set_format(vk::Format::eR8G8B8A8Unorm);
        tex->set_mipmap_levels(1);
        tex->create();
        tex->create_image_view(vk::ImageAspectFlagBits::eColor);


    // 3. Map the buffer to memory and copy the image to it.
        void * image_buffer_data = staging_buffer->map_memory();
        memcpy( image_buffer_data, D.data(), D.size() );
        staging_buffer->unmap_memory();


    // 4. Now that the data is on the device. We need to get it from teh buffer
    //    to the texture. To do this we will record a command buffer to do the
    //    following:
    //         a. convert the texture2d into a layout which can accept transfer data
    //         b. copy the data from teh buffer to the texture2d.
    //         c. convert the texture2d into a layout which is good for shader use

        // allocate the command buffer
        auto cb1 = cp->AllocateCommandBuffer();
        cb1.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );

            // a. convert the texture to eTransferDstOptimal
            tex->convert_layer(cb1, vk::ImageLayout::eTransferDstOptimal,0,0);

            // b. copy the data from the buffer to the texture
            vk::BufferImageCopy BIC;
            BIC.setBufferImageHeight(  D.height() )
               .setBufferOffset(0)
               .setImageExtent( vk::Extent3D(D.width(), D.height(), 1) )
               .setImageOffset( vk::Offset3D(0,0,0))
               .imageSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
                                .setBaseArrayLayer(0)
                                .setLayerCount(1)
                                .setMipLevel(0);

            tex->copy_buffer( cb1, staging_buffer, BIC);

            // c. convert the texture into eShaderReadOnlyOptimal
            tex->convert(cb1, vk::ImageLayout::eShaderReadOnlyOptimal);

        // end and submit the command buffer
        cb1.end();
        C.submit_cmd_buffer(cb1);
        // free the command buffer
        cp->FreeCommandBuffer(cb1);
//==============================================================================

    set->attach_sampler(0, tex);
    set->update();


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
            cb.bindVertexBuffers(0, vertex_buffer->get(), {0} );// ( m_VertexBuffer, 0);
            cb.bindIndexBuffer(  index_buffer ->get() , 0 , vk::IndexType::eUint16);
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
