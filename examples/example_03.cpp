/*
 * Example 3: Dynamic Uniform Buffers
 *
 * This example demonstrates how to setup a rendering pipeline using depth
 * testing. Depth testing is an integral part of most rendering pipelines.
 *
 */



#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <vka/core/image.h>
#include <vka/vka.h>


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define WIDTH 1024
#define HEIGHT 768
#define APP_TITLE "Example_03 - Dynamic Uniform Buffers"

// This is the vertex structure we are going to use
// it contains a position and a UV coordates field
struct Vertex
{
    glm::vec3 p; // position
    glm::vec2 u; // uv coords
    glm::vec3 n; // normal
};

// This is the structure of the uniform buffer we want.
// it needs to match the structure in the shader.
struct uniform_buffer_t
{
    glm::mat4 view;
    glm::mat4 proj;
};

struct dynamic_uniform_buffer_t
{
    glm::mat4 model;
};

/**
 * @brief get_elapsed_time
 * @return
 *
 * Gets the number of seconds since the application started.
 */
float get_elapsed_time()
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    double time = 1.0 * std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0;

    return time;

}


/**
 * @brief create_box_mesh
 * @param dx - dimension of the box
 * @param dy - dimension of the box
 * @param dz - dimension of the box
 * @param vertices
 * @param indices
 *
 * Create a box mesh and save the vertices and indices in the input vectors.
 * The vertices/indices will then be copied into the graphics's buffers
 */
void create_box_mesh(float dx , float dy , float dz , std::vector<Vertex> & vertices, std::vector<uint16_t> & indices );



int main(int argc, char ** argv)
{
    //==========================================================================
    // 1. Initlize the library and create a window
    //==========================================================================
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE,  GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, APP_TITLE, nullptr, nullptr);


    // the context is the main class for the vka library. It is keeps track of
    // all the vulkan objects and releases them appropriately when it is destroyed
    // It is also responsible for creating the objects such as buffers, textures
    // command pools, etc.
    vka::context C;

    C.init();
    C.create_window_surface(window); // create the vulkan surface using the window provided
    C.create_device(); // find the appropriate device
    C.create_swap_chain( {WIDTH,HEIGHT}); // create the swap chain

    //==========================================================================



    //==========================================================================
    // Create the Render pass and the frame buffers.
    //
    // The Context holds the images that will be used for the swap chain. We need
    // to create the framebuffers with those images.
    //
    // All objects created by the context requires a unique name
    //==========================================================================




    // Create a depth texture which we will use to be store the depths
    // of each pixel.
    auto * depth = C.new_depth_texture("depth_texture");
    depth->set_size( WIDTH, HEIGHT, 1);
    depth->create();
    depth->create_image_view( vk::ImageAspectFlagBits::eDepth);

    // Convert the depth texture into the proper image layout.
    // This method will automatically allocate a a command buffer and
    // and then submit it.  Alternatively, passing in a command buffer
    // as the first argument simply writes the command to the buffer
    // but does not submit it.
    depth->convert(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vka::renderpass * R = C.new_renderpass("main_renderpass");
    R->attach_color(vk::Format::eB8G8R8A8Unorm);
    R->attach_depth( depth->get_format() );  // [NEW] we will now be using a depth attachment
    R->create(C);


    std::vector<vka::framebuffer*> framebuffers;

    std::vector<vk::ImageView> & iv = C.get_swapchain_imageviews();
    int i=0;
    for(auto & view : iv)
    {
        framebuffers.push_back(  C.new_framebuffer( std::string("fb_") + std::to_string(i++) ) );
        framebuffers.back()->create( *R, vk::Extent2D{WIDTH,HEIGHT}, view,
                                     depth->get_image_view()); // [NEW]
    }
    //==========================================================================



    //==========================================================================
    // Initialize the Command and Descriptor Pools
    //==========================================================================
    vka::descriptor_pool* descriptor_pool = C.new_descriptor_pool("main_desc_pool");
    descriptor_pool->set_pool_size(vk::DescriptorType::eCombinedImageSampler, 2);
    descriptor_pool->set_pool_size(vk::DescriptorType::eUniformBuffer, 1);
    // [NEW]
    descriptor_pool->set_pool_size(vk::DescriptorType::eUniformBufferDynamic, 1);

    descriptor_pool->create();

    vka::command_pool* cp = C.new_command_pool("main_command_pool");
    //==========================================================================






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

        std::vector<Vertex>   vertices;
        std::vector<uint16_t> indices;

        create_box_mesh( 1,1,1, vertices, indices);

        // Create two buffers, one for vertices and one for indices. THey
        // will each be 1024 bytes long
        vka::buffer* vertex_buffer = C.new_vertex_buffer(  "vb", 5*1024 );
        vka::buffer* index_buffer  = C.new_index_buffer(   "ib", 5*1024 );
        vka::buffer* u_buffer      = C.new_uniform_buffer( "ub", 5*1024);

        // [NEW]
        vka::buffer* du_buffer     = C.new_uniform_buffer( "dub", 5*1024);



        // allocate a staging buffer of 10MB
        vka::buffer * staging_buffer = C.new_staging_buffer( "sb", 1024*1024*10 );

        // using the map< > method, we can return an array_view into the
        // memory. We are going to place them in their own scope so that
        // the array_view is destroyed after exiting the scope. This is
        // so we do not accidenty access the array_view after the
        // staging_buffer has been unmapped.
        {
            void * vertex_map =  staging_buffer->map_memory();
            memcpy( vertex_map, vertices.data(), vertices.size()*sizeof(Vertex));

            LOG << "Size of Vertices: " << vertices.size()*sizeof(Vertex) << ENDL;

            void * index_map = static_cast<char*>(vertex_map) + vertices.size()*sizeof(Vertex);

            memcpy( index_map, indices.data(), indices.size()*sizeof(uint16_t));

            LOG << "Size of Indices: " << indices.size()*sizeof(uint16_t) << ENDL;
        }

        // 2. Copy the data from the host-visible buffer to the vertex/index buffers

        // allocate a comand buffer
        vk::CommandBuffer copy_cmd = cp->AllocateCommandBuffer();
        copy_cmd.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );

        // write the commands to copy each of the buffer data
        const vk::DeviceSize vertex_offset   = 0;
        const vk::DeviceSize vertex_size     = vertices.size()*sizeof(Vertex);

        const vk::DeviceSize index_offset    = vertices.size()*sizeof(Vertex);
        const vk::DeviceSize index_size      = indices.size()*sizeof(uint16_t);


        copy_cmd.copyBuffer( *staging_buffer , *vertex_buffer, vk::BufferCopy{ vertex_offset    , 0 , vertex_size } );
        copy_cmd.copyBuffer( *staging_buffer , *index_buffer , vk::BufferCopy{ index_offset     , 0 , index_size  } );

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

    // 1. First load host_image into memory, and specifcy we want 4 channels.
        vka::host_image D("../resources/textures/Brick-2852a.jpg",4);


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


    // 4. Now that the data is on the device. We need to get it from the buffer
    //    to the texture. To do this we will record a command buffer to do the
    //    following:
    //         a. convert the texture2d into a layout which can accept transfer data
    //         b. copy the data from the buffer to the texture2d.
    //         c. convert the texture2d into a layout which is good for shader use

        // allocate the command buffer
        vk::CommandBuffer cb1 = cp->AllocateCommandBuffer();
        cb1.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );

            // a. convert the texture to eTransferDstOptimal
            tex->convert_layer(cb1, vk::ImageLayout::eTransferDstOptimal,0,0);

            // b. copy the data from the buffer to the texture
            vk::BufferImageCopy BIC;
            BIC.setBufferImageHeight(  D.height() )
               .setBufferOffset(0) // the image data starts at the start of the buffer
               .setImageExtent( vk::Extent3D(D.width(), D.height(), 1) ) // size of the image
               .setImageOffset( vk::Offset3D(0,0,0)) // where in the texture we want to paste the image
               .imageSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
                                .setBaseArrayLayer(0) // the layer to copy
                                .setLayerCount(1) // only copy one layer
                                .setMipLevel(0);  // only the first mip-map level

            tex->copy_buffer( cb1, staging_buffer, BIC);

            // c. convert the texture into eShaderReadOnlyOptimal
            tex->convert(cb1, vk::ImageLayout::eShaderReadOnlyOptimal);

        // end and submit the command buffer
        cb1.end();
        C.submit_cmd_buffer(cb1);
        // free the command buffer
        cp->FreeCommandBuffer(cb1);
//==============================================================================


//==============================================================================
// Create a Rendering pipeline
//
//==============================================================================
        // create the vertex shader from a pre compiled SPIR-V file
        vka::shader* vertex_shader = C.new_shader_module("vs");
        vertex_shader->load_from_file("../resources/shaders/dynamic_uniform_buffer/dynamic_uniform_buffer_v.spv");

        // create the fragment shader from a pre compiled SPIR-V file
        vka::shader* fragment_shader = C.new_shader_module("fs");
        fragment_shader->load_from_file("../resources/shaders/dynamic_uniform_buffer/dynamic_uniform_buffer_f.spv");

        vka::pipeline* pipeline = C.new_pipeline("triangle");

        // Create the graphics Pipeline
          pipeline->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                  ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

                  ->set_vertex_shader(   vertex_shader )   // the shaders we want to use
                  ->set_fragment_shader( fragment_shader ) // the shaders we want to use

                  // tell the pipeline that attribute 0 contains 3 floats
                  // and the data starts at offset 0
                  ->set_vertex_attribute(0 ,  offsetof(Vertex,p),  vk::Format::eR32G32B32Sfloat,  sizeof(Vertex) )
                  // tell the pipeline that attribute 1 contains 3 floats
                  // and the data starts at offset 12
                  ->set_vertex_attribute(1 , offsetof(Vertex,u),  vk::Format::eR32G32Sfloat,  sizeof(Vertex) )

                  ->set_vertex_attribute(2 , offsetof(Vertex,n),  vk::Format::eR32G32B32Sfloat,  sizeof(Vertex) )

                  // Triangle vertices are drawn in a counter clockwise manner
                  // using the right hand rule which indicates which face is the
                  // front
                  ->set_front_face(vk::FrontFace::eCounterClockwise)

                  // Cull all back facing triangles.
                  ->set_cull_mode(vk::CullModeFlagBits::eNone)

                  // Tell the shader that we are going to use a texture
                  // in Set #0 binding #0
                  ->add_texture_layout_binding(0, 0, vk::ShaderStageFlagBits::eFragment)

                  // Tell teh shader that we are going to use a uniform buffer
                  // in Set #0 binding #0
                  ->add_uniform_layout_binding(1, 0, vk::ShaderStageFlagBits::eVertex)

                  // Tell teh shader that we are going to use a uniform buffer
                  // in Set #0 binding #0
                  ->add_dynamic_uniform_layout_binding(2, 0, vk::ShaderStageFlagBits::eVertex)
                  //
                  ->set_render_pass( R )
                  ->create();



//==============================================================================
// Create a descriptor set:
//   once the pipeline has been created. We need to create a descriptor set
//   which we can use to tell what textures we want to use in the shader.
//   The pipline object can generate a descriptor set for you.
//==============================================================================
    // we want a descriptor set for set #0 in the pipeline.
    vka::descriptor_set * texture_descriptor = pipeline->create_new_descriptor_set(0, descriptor_pool);
    //  attach our texture to binding 0 in the set.
    texture_descriptor->attach_sampler(0, tex);
    texture_descriptor->update();

    vka::descriptor_set * ubuffer_descriptor = pipeline->create_new_descriptor_set(1, descriptor_pool);
    ubuffer_descriptor->attach_uniform_buffer(0, u_buffer, sizeof(uniform_buffer_t), 0);
    ubuffer_descriptor->update();

    vka::descriptor_set * dubuffer_descriptor = pipeline->create_new_descriptor_set(2, descriptor_pool);
    dubuffer_descriptor->attach_dynamic_uniform_buffer(0, du_buffer, sizeof(dynamic_uniform_buffer_t), 0);
    dubuffer_descriptor->update();

    vka::array_view<uniform_buffer_t> staging_buffer_map        = staging_buffer->map<uniform_buffer_t>();
    vka::array_view<dynamic_uniform_buffer_t> staging_dbuffer_map = staging_buffer->map<dynamic_uniform_buffer_t>(sizeof(uniform_buffer_t));

    vk::CommandBuffer cb = cp->AllocateCommandBuffer();


    vka::semaphore * image_available_semaphore = C.new_semaphore("image_available_semaphore");
    vka::semaphore * render_complete_semaphore = C.new_semaphore("render_complete_semaphore");



    // Dynamic Uniform buffers have a set alignment, meaning the number of bytes
    // bound to the pipeline must be a multiple of the alignment
    // In most case the alignment is 256 bytes.
    auto alignment = C.get_physical_device_limits().minUniformBufferOffsetAlignment;

    //==========================================================================
    // Perform the Rendering
    //==========================================================================
    // Here we finally perform the main loop for rendering.
    // The steps required are:
    //    a. update the uniform buffer with the MVP matrices
    //
    while (!glfwWindowShouldClose(window) )
    {
       float t = get_elapsed_time();
      // Get the next available image in the swapchain
      uint32_t fb_index = C.get_next_image_index(image_available_semaphore);
      glfwPollEvents();

      // reset the command buffer so that we can record from scratch again.
      cb.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
      cb.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse) );



      // The staging buffer will hold all data that will be sent to
      // the other two buffers.
      //
      // +--------------+------+--------------------------------+
      // | uniform_data | obj1 | obj2 |                         | Staging Buffer
      // +--------------+------+--------------------------------+
      uint32_t ub_src_offset = 0;
      uint32_t ub_size       = sizeof(uniform_buffer_t);

      std::vector<uint32_t> dub_src_offset = { ub_size ,
                                               ub_size + sizeof(dynamic_uniform_buffer_t)};





      #define MAX_OBJECTS 2
      // Copy the uniform buffer data into the staging buffer
      const float AR = WIDTH / ( float )HEIGHT;
      staging_buffer_map[0].view        = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
      staging_buffer_map[0].proj        = glm::perspective(glm::radians(45.0f), AR, 0.1f, 10.0f);
      staging_buffer_map[0].proj[1][1] *= -1;

      // Copy the dynamic uniform buffer data into the staging buffer
      staging_dbuffer_map[0].model       =  glm::rotate(glm::mat4(), t * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::translate( glm::mat4(), glm::vec3(-1,0,0) ) ;
      staging_dbuffer_map[1].model       =  glm::rotate(glm::mat4(), t * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::translate( glm::mat4(), glm::vec3(1,0,0));


      // +------------------------------------------------------+
      // | uniform_data |                                       | Uniform Buffer
      // +------------------------------------------------------+


      // Copy the uniform buffer data from the stating buffer to the uniform buffer. THis normally only needs to be done
      // once per rendering frame because it contains frame constant data.
      cb.copyBuffer( *staging_buffer ,  *u_buffer , vk::BufferCopy{ 0,0,sizeof(uniform_buffer_t) } );


      // Copy the dynamic uniform buffer data from the staging buffer
      // to the appropriate offset in the Dynamic Uniform Buffer.
      // +-------------+---------------------------------------+
      // | obj1        | obj2         | obj3...                | Dynamic Uniform Buffer
      // +-------------+---------------------------------------+
      // |<-alignment->|

      for(uint32_t j=0; j < MAX_OBJECTS; j++)
      {
          // byte offset within the staging buffer where teh data resides
          auto srcOffset = sizeof(uniform_buffer_t) + j * sizeof(dynamic_uniform_buffer_t);
          // byte offset within the dynamic uniform buffer where to copy the data
          auto dstOffset = j * alignment;
          // number of bytes to copy
          auto size      = sizeof(dynamic_uniform_buffer_t);

          cb.copyBuffer( *staging_buffer , *du_buffer , vk::BufferCopy{ srcOffset,dstOffset, size } );
      }


      //  BEGIN RENDER PASS=====================================================
      // We want the to use the render pass we created earlier
      vk::RenderPassBeginInfo renderPassInfo;
      renderPassInfo.renderPass        = *R;
      renderPassInfo.framebuffer       = *framebuffers[fb_index];
      renderPassInfo.renderArea.offset = vk::Offset2D{0,0};

      renderPassInfo.renderArea.extent = vk::Extent2D(WIDTH,HEIGHT);

      // Clear values are used to clear the various frame buffers
      // we want to clear the colour values to black
      // at the start of rendering.
      std::vector<vk::ClearValue> clearValues;
      clearValues.push_back( vk::ClearValue( vk::ClearColorValue( std::array<float,4>{0.0f, 0.f, 0.f, 1.f} ) ) );
      clearValues.push_back(vk::ClearValue( vk::ClearDepthStencilValue(1.0f,0) ) );

      renderPassInfo.clearValueCount = clearValues.size();
      renderPassInfo.pClearValues    = clearValues.data();

      cb.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
      //========================================================================


      // bind the pipeline that we want to use next
        cb.bindPipeline( vk::PipelineBindPoint::eGraphics, *pipeline );

      // bind the two descriptor sets that we need to that pipeline
       cb.bindDescriptorSets( vk::PipelineBindPoint::eGraphics,
                                                    pipeline->get_layout(),
                                                    0,
                                                    vk::ArrayProxy<const vk::DescriptorSet>( texture_descriptor->get()),
                                                    nullptr );

        cb.bindDescriptorSets( vk::PipelineBindPoint::eGraphics,
                                                pipeline->get_layout(),
                                                1,
                                                vk::ArrayProxy<const vk::DescriptorSet>( ubuffer_descriptor->get()),
                                                nullptr );

    // bind the vertex/index buffers
        cb.bindVertexBuffers(0, vertex_buffer->get(), {0} );
        cb.bindIndexBuffer(  index_buffer->get() , 0 , vk::IndexType::eUint16);

      //========================================================================
      // Draw all the objects while binding the dynamic uniform buffer
      // to
      //========================================================================
      for(uint32_t j=0 ; j < MAX_OBJECTS; j++)
      {
            cb.bindDescriptorSets( vk::PipelineBindPoint::eGraphics,
                                                    pipeline->get_layout(),
                                                    2,
                                                    vk::ArrayProxy<const vk::DescriptorSet>( dubuffer_descriptor->get()),
                                                    vk::ArrayProxy<const uint32_t>(j*alignment) );



    // draw 3 indices, 1 time, starting from index 0, using a vertex offset of 0
            cb.drawIndexed(36, 1, 0 , 0, 0);
      }



      cb.endRenderPass();
      cb.end();

      // Submit the command buffers, but wait until the image_available_semaphore
      // is flagged. Once the commands have been executed, flag the render_complete_semaphore
      C.submit_command_buffer(cb, image_available_semaphore, render_complete_semaphore);

      // present the image to the surface, but wait for the render_complete_semaphore
      // to be flagged by the submit_command_buffer
      C.present_image(fb_index, render_complete_semaphore);


      std::this_thread::sleep_for( std::chrono::milliseconds(3) );
    }

    return 0;
}

#define STB_IMAGE_IMPLEMENTATION
#include<stb/stb_image.h>



void create_box_mesh(float dx , float dy , float dz , std::vector<Vertex> & vertices, std::vector<uint16_t> & indices )
{

    indices .clear();
    vertices.clear();

    using namespace glm;


    std::vector<uint16_t> I;
//       |       Position                           |   UV         |     Normal    |
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz} , {0.0,0.0}  ,  { 0.0,  0.0,  1.0} } );   // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz} , {1.0,0.0}  ,  { 0.0,  0.0,  1.0} } );   // 1
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz} , {1.0,1.0}  ,  { 0.0,  0.0,  1.0} } );   // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz} , {0.0,0.0}  ,  { 0.0,  0.0,  1.0} } );   // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz} , {1.0,1.0}  ,  { 0.0,  0.0,  1.0} } );   // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz} , {0.0,1.0}  ,  { 0.0,  0.0,  1.0} } );   // 3
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz} , {0.0,1.0}  ,  { 0.0,  0.0, -1.0} } ); // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz} , {1.0,1.0}  ,  { 0.0,  0.0, -1.0} } ); // 1
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz} , {1.0,0.0}  ,  { 0.0,  0.0, -1.0} } ); // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz} , {0.0,1.0}  ,  { 0.0,  0.0, -1.0} } ); // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz} , {1.0,0.0}  ,  { 0.0,  0.0, -1.0} } ); // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz} , {0.0,0.0}  ,  { 0.0,  0.0, -1.0} } ); // 3
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz} , {0.0,0.0}  ,  {-1.0f, 0.0,  0.0} } );  // 0
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz} , {1.0,0.0}  ,  {-1.0f, 0.0,  0.0} } );  // 1
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz} , {1.0,1.0}  ,  {-1.0f, 0.0,  0.0} } );  // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz} , {0.0,0.0}  ,  {-1.0f, 0.0,  0.0} } );  // 0
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz} , {1.0,1.0}  ,  {-1.0f, 0.0,  0.0} } );  // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz} , {0.0,1.0}  ,  {-1.0f, 0.0,  0.0} } );  // 3
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz} , {0.0,1.0}  ,  { 1.0f, 0.0,  0.0} } ); // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz} , {1.0,1.0}  ,  { 1.0f, 0.0,  0.0} } ); // 1
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz} , {1.0,0.0}  ,  { 1.0f, 0.0,  0.0} } ); // 2
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz} , {0.0,1.0}  ,  { 1.0f, 0.0,  0.0} } ); // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz} , {1.0,0.0}  ,  { 1.0f, 0.0,  0.0} } ); // 2
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz} , {0.0,0.0}  ,  { 1.0f, 0.0,  0.0} } ); // 3
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz} , {0.0,0.0}  ,  { 0.0f,-1.0,  0.0} } ); // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz} , {1.0,0.0}  ,  { 0.0f,-1.0,  0.0} } ); // 1
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz} , {1.0,1.0}  ,  { 0.0f,-1.0,  0.0} } ); // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz} , {0.0,0.0}  ,  { 0.0f,-1.0,  0.0} } ); // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz} , {1.0,1.0}  ,  { 0.0f,-1.0,  0.0} } ); // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz} , {0.0,1.0}  ,  { 0.0f,-1.0,  0.0} } ); // 3
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz} , {0.0,1.0}  ,  { 0.0f, 1.0,  0.0} } ); // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz} , {1.0,1.0}  ,  { 0.0f, 1.0,  0.0} } ); // 1
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz} , {1.0,0.0}  ,  { 0.0f, 1.0,  0.0} } ); // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz} , {0.0,1.0}  ,  { 0.0f, 1.0,  0.0} } ); // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz} , {1.0,0.0}  ,  { 0.0f, 1.0,  0.0} } ); // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz} , {0.0,0.0}  ,  { 0.0f, 1.0,  0.0} } ); // 3

    //=========================
    // Edges of the triangle : postion delta


    //=========================
    for(uint16_t i=0;i<36;i++)
        indices.push_back(i);


}
