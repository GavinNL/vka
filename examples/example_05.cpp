/*
 * Example 5: Mip Maps
 *
 * Mipmaps are a downsampled version of a texture. If a square texture has
 * dimensions 512x512, then a level 1 mipmap woudl be 256x256, a level 2 would
 * be 128x128, and so forth all the way down to 1x1.
 *
 * Mipmaps are used when rendering objects far from the camera, if the texture
 * is too detailed, then when an object is far away, two pixels close together
 * might actually be very far apart on the texture. This create large colour
 * variances in objects farther away.
 *
 * When objects are close to the camera, the level 0 mipmap is used at full
 * resoluation. As the object moves farther away, lower levels are used.
 *
 * This example demonstrates how to generate mipmaps for a texture array.
 *
 * We can let the shader choose what mipmap level to use automatically (recomended)
 * or we can manually choose the level we want. We are going to add a
 * variable in the Push Constants to indicate what leve we want, or if we choose
 * -1, we will let the shader choose.
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
#define APP_TITLE "Example_05 - Mip Maps"

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


// Each object just needs the model matrix
struct dynamic_uniform_buffer_t
{
    glm::mat4 model;
};

// This data will be written directly to the command buffer to
// be passed to the shader as a push constant.
struct push_constants_t
{
    int index; // index into the texture array layer
    int miplevel; // mipmap level we want to use, -1 for shader chosen.
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


uint32_t tick()
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime).count();

    //return time;
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
    auto surface = C.create_window_surface(window); // create the vulkan surface using the window provided
    C.create_device(); // find the appropriate device

    auto * screen = C.new_screen("screen");
    screen = C.new_screen("m_win");
    screen->set_extent( vk::Extent2D(WIDTH,HEIGHT) );
    screen->set_surface( surface );
    screen->create();

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
// Create the Texture2dArray
//
//==============================================================================

    // 1. First load host_image into memory, and specifcy we want 4 channels.
        vka::host_image D("resources/textures/Brick-2852a.jpg",4);
        vka::host_image D2("resources/textures/noise.jpg",4);



    // 2. Use the context's helper function to create a device local texture
    //    We will be using a texture2d which is a case specific version of the
    //    generic texture
        vka::texture2darray * tex = C.new_texture2darray("test_texture");
        tex->set_size( D.width() , D.height() );
        tex->set_format(vk::Format::eR8G8B8A8Unorm);
        tex->set_mipmap_levels(8);
        tex->set_layers(10);
        tex->create();
        tex->create_image_view(vk::ImageAspectFlagBits::eColor);


#if 0
        auto t1 = tick();
        tex->copy_image( D , 0, vk::Offset2D(0,0) );
        tex->copy_image( D2, 1, vk::Offset2D(0,0) );

        LOG << "Time to copy image: " << (tick()-t1) << ENDL;
#else
    // 3. Map the buffer to memory and copy the the two images to it
    //    one right after the other.
    //
    //  Because we only have 2 textures to copy, both will easily fit in the
    //  staging buffer. If you have many textures, you will have to run
    //  this section multiple times: write to staging buffer, copy to array
    //   write to staging buffer, copy to array. etc.
        auto t1 = tick();
        void * image_buffer_data = staging_buffer->map_memory();
        memcpy( image_buffer_data, D.data(), D.size() );


        image_buffer_data = static_cast<char*>(image_buffer_data) + D.size();
        memcpy( image_buffer_data, D2.data(), D2.size() );

        staging_buffer->unmap_memory();


    // 4. Now that the data is on the device. We need to get it from the buffer
    //    to the texture array. To do this we will record a command buffer to do the
    //    following:
    //         a. Convert the layers in the texture array into a layout which can accept transfer data
    //         b. copy the data from the buffer to the texture2darray.
    //         c. convert the layers into a layout which is good for shader use

        // allocate the command buffer
        vk::CommandBuffer cb1 = cp->AllocateCommandBuffer();
        cb1.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );


            // a. convert the texture to eTransferDstOptimal
            tex->convert_layer(cb1, vk::ImageLayout::eTransferDstOptimal,0,0);
            tex->convert_layer(cb1, vk::ImageLayout::eTransferDstOptimal,1,0);

            // b. copy the data from the buffer to the texture
            vk::BufferImageCopy BIC;
            BIC.setBufferImageHeight(  D.height() )
               .setBufferOffset(0) // the image data starts at the start of the buffer
               .setImageExtent( vk::Extent3D(D.width(), D.height(), 1) ) // size of the image
               .setImageOffset( vk::Offset3D(0,0,0)) // where in the texture we want to paste the image
               .imageSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
                                .setBaseArrayLayer(0) // the layer to copy
                                .setLayerCount(2) // only copy 2 layers
                                .setMipLevel(0);  // only the first mip-map level

            tex->copy_buffer( cb1, staging_buffer, BIC);



            //==================================================================
            // Generate the mipmaps manually for layer 0
            //==================================================================
            for(uint32_t i=0; i< tex->get_mipmap_levels()-1 ; i++)
            {
                // convert the upper layer to be a Transfer Source
                tex->convert_layer(cb1, vk::ImageLayout::eTransferSrcOptimal,0, i);
                // convert the next lower level to be transfer dest
                tex->convert_layer(cb1, vk::ImageLayout::eTransferDstOptimal,0, i+1);

                // copy from the above layer down one
                tex->blit_mipmap(cb1, 0,  // copy from layer 0
                                      0,  // to layer 0
                                      i,  // from mipmap i
                                      i+1); //  to mipmap i+1

                // convert the upper layer back to eShaderReadOnlyOptimal
                tex->convert_layer(cb1, vk::ImageLayout::eShaderReadOnlyOptimal, 0, i);
            }
            // convert the last layer to eShaderReadOnlyOptimal
            tex->convert_layer(cb1, vk::ImageLayout::eShaderReadOnlyOptimal, 0, tex->get_mipmap_levels()-1);
            //==================================================================


            //==================================================================
            // Generate mip maps automatically using the texture classes
            // built in method. This method does the same as the above.
            //==================================================================
            tex->generate_mipmaps(cb1,1);
            //==================================================================


        // end and submit the command buffer
        cb1.end();
        C.submit_cmd_buffer(cb1);
        // free the command buffer
        cp->FreeCommandBuffer(cb1);
        LOG << "Time to copy image: " << (tick()-t1) << ENDL;
#endif
//==============================================================================


//==============================================================================
// Create a Rendering pipeline
//
//==============================================================================
        // create the vertex shader from a pre compiled SPIR-V file
        vka::shader* vertex_shader = C.new_shader_module("vs");
        vertex_shader->load_from_file("resources/shaders/mipmaps/mipmaps_v.spv");

        // create the fragment shader from a pre compiled SPIR-V file
        vka::shader* fragment_shader = C.new_shader_module("fs");
        fragment_shader->load_from_file("resources/shaders/mipmaps/mipmaps_f.spv");

        vka::pipeline* pipeline = C.new_pipeline("triangle");

        // Create the graphics Pipeline
          pipeline->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                  ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

                  ->set_vertex_shader(   vertex_shader )   // the shaders we want to use
                  ->set_fragment_shader( fragment_shader ) // the shaders we want to use

                  // tell the pipeline that attribute 0 contains 3 floats
                  // and the data starts at offset 0
                  ->set_vertex_attribute(0, 0 ,  offsetof(Vertex,p),  vk::Format::eR32G32B32Sfloat,  sizeof(Vertex) )
                  // tell the pipeline that attribute 1 contains 3 floats
                  // and the data starts at offset 12
                  ->set_vertex_attribute(0, 1 , offsetof(Vertex,u),  vk::Format::eR32G32Sfloat,  sizeof(Vertex) )

                  ->set_vertex_attribute(0, 2 , offsetof(Vertex,n),  vk::Format::eR32G32B32Sfloat,  sizeof(Vertex) )

                  // Triangle vertices are drawn in a counter clockwise manner
                  // using the right hand rule which indicates which face is the
                  // front
                  ->set_front_face(vk::FrontFace::eCounterClockwise)

                  // Cull all back facing triangles.
                  ->set_cull_mode(vk::CullModeFlagBits::eBack)

                  // Tell the shader that we are going to use a texture
                  // in Set #0 binding #0
                  ->add_texture_layout_binding(0, 0, vk::ShaderStageFlagBits::eFragment)

                  // Tell teh shader that we are going to use a uniform buffer
                  // in Set #0 binding #0
                  ->add_uniform_layout_binding(1, 0, vk::ShaderStageFlagBits::eVertex)

                  // Tell teh shader that we are going to use a uniform buffer
                  // in Set #0 binding #0
                  ->add_dynamic_uniform_layout_binding(2, 0, vk::ShaderStageFlagBits::eVertex)

                  // Add a push constant to the layout. It is accessable in the vertex shader
                  // stage only.
                  ->add_push_constant( sizeof(push_constants_t), 0, vk::ShaderStageFlagBits::eVertex)
                  //
                  ->set_render_pass( screen->get_renderpass() )
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

    vka::command_buffer cb = cp->AllocateCommandBuffer();


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
      staging_buffer_map[0].view        = glm::lookAt(glm::vec3(0.0f, 2.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
      staging_buffer_map[0].proj        = glm::perspective(glm::radians(45.0f), AR, 0.1f, 100.0f);
      staging_buffer_map[0].proj[1][1] *= -1;

      // Copy the dynamic uniform buffer data into the staging buffer
      staging_dbuffer_map[0].model       =  glm::rotate(glm::mat4(), t * glm::radians(30.0f), glm::vec3(1.0f, -2.0f, 1.0f))
                                           *glm::rotate(glm::mat4(), t * glm::radians(60.0f), glm::vec3(-4.0f, -2.0f, 1.3f))*glm::translate( glm::mat4(), glm::vec3( 1,0,0) );;

      staging_dbuffer_map[1].model       =  glm::rotate(glm::mat4(), t * glm::radians(30.0f), glm::vec3(1.0f, -2.0f, 1.0f))
                                           *glm::rotate(glm::mat4(), t * glm::radians(60.0f), glm::vec3(-4.0f, -2.0f, 1.3f))*glm::translate( glm::mat4(), glm::vec3(-1,0,0) );


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



      uint32_t frame_index = screen->prepare_next_frame(image_available_semaphore);
      screen->beginRender(cb, frame_index);

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
          // Here we write the data to the command buffer.
          push_constants_t push;
          push.index    = 0;

       //   fmod( get_elapsed_time(), 2);
          if(j==0)
          {
            push.miplevel = -1;
          } else {
            push.miplevel = (int)fmod( t ,tex->get_mipmap_levels());
          }

          cb.pushConstants( pipeline->get_layout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(push_constants_t), &push);

          cb.bindDescriptorSets( vk::PipelineBindPoint::eGraphics,
                                                  pipeline->get_layout(),
                                                  2,
                                                  vk::ArrayProxy<const vk::DescriptorSet>( dubuffer_descriptor->get()),
                                                  vk::ArrayProxy<const uint32_t>(j*alignment) );



    // draw 3 indices, 1 time, starting from index 0, using a vertex offset of 0
            cb.drawIndexed(36, 1, 0 , 0, 0);
      }


      screen->endRender(cb);
      cb.end();

      // Submit the command buffers, but wait until the image_available_semaphore
      // is flagged. Once the commands have been executed, flag the render_complete_semaphore
      C.submit_command_buffer(cb, image_available_semaphore, render_complete_semaphore);

      // present the image to the surface, but wait for the render_complete_semaphore
      // to be flagged by the submit_command_buffer
      screen->present_frame( frame_index, render_complete_semaphore);

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
