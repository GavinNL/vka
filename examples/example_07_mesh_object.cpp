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
 * variable in the Push Constants to indicate what level we want, or if we choose
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
#include <vka/core/screen_target.h>
#include <vka/vka.h>

#include <vka/core/primatives.h>

#include <vka/core2/BufferMemoryPool.h>
#include <vka/core2/TextureMemoryPool.h>
#include <vka/core2/MeshObject.h>

#include <vka/linalg.h>

#define WIDTH 1024
#define HEIGHT 768
#define APP_TITLE "Example_07 - Mesh Objects"


// This is the structure of the uniform buffer we want.
// it needs to match the structure in the shader.
struct per_frame_uniform_t
{
    glm::mat4 view;
    glm::mat4 proj;
};
using uniform_buffer_t = per_frame_uniform_t;


// This data will be written directly to the command buffer to
// be passed to the shader as a push constant.
struct push_constants_t
{
    glm::mat4 model;
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

vka::MeshObject HostToGPU( vka::host_mesh & host_mesh ,
                           vka::BufferMemoryPool & BufferPool,
                           vka::BufferMemoryPool & StagingBufferPool,
                           vka::command_pool * cp,
                           vka::context & C)
{

    vka::host_mesh & CubeMesh = host_mesh;

    assert(  CubeMesh.has_attribute( vka::VertexAttribute::ePosition ) );
    assert(  CubeMesh.has_attribute( vka::VertexAttribute::eUV       ) );
    assert(  CubeMesh.has_attribute( vka::VertexAttribute::eNormal   ) );
    assert(  CubeMesh.has_attribute( vka::VertexAttribute::eIndex    ) );

    //=====================================================================

    // A MeshObject is essentially just a container that holde
    // information about a Mesh. It contains multiple SubBuffers; one for
    // each attribute in the mesh (eg: position, UV, normals)
    // and an additional optional buffer for Indices.
    //
    // MeshObjects can be bound using a command buffer similarly to how
    // a vertex or index buffer is bound. Under the hood, it simply
    // binds the individual buffers to their appropriate bind index.
    vka::MeshObject CubeObj;

    vka::VertexAttribute Attr[] =
    {
        vka::VertexAttribute::ePosition ,
        vka::VertexAttribute::eUV,
        vka::VertexAttribute::eNormal,
    };

    // We're going to keep a vector of staging buffers so that
    // they do not deallocate themselves until after we are
    // done with them.
    std::vector<vka::SubBuffer_p>    staging_buffers;


    vka::command_buffer copy_cmd = cp->AllocateCommandBuffer();
    copy_cmd.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit ) );

    for(uint32_t i =0; i < 3 ; i++)
    {
        auto & P = CubeMesh.get_attribute( Attr[i] );

        const auto byte_size = P.data_size();

        CubeObj.AddAttributeBuffer(i,  BufferPool.NewSubBuffer( byte_size) );


        // Create a staging buffer for copying attribute i;
        auto S = StagingBufferPool.NewSubBuffer( byte_size );

        // copy data to staging buffer
        S->CopyData( P.data(), byte_size );

        // keep a reference to the staging buffer so it doesn't
        // get erased.
        staging_buffers.push_back(S);

        copy_cmd.copySubBuffer( S, CubeObj.GetAttributeBuffer(i), vk::BufferCopy{ 0 , 0 , byte_size } );
    }

    {
        auto & I = CubeMesh.get_attribute( vka::VertexAttribute::eIndex );

        CubeObj.AddIndexBuffer( vk::IndexType::eUint16,  BufferPool.NewSubBuffer(I.data_size()) );

        auto byte_size = I.data_size();
        auto S = StagingBufferPool.NewSubBuffer( byte_size );

        // copy data to staging buffer
        S->CopyData( I.data(), byte_size );

        // keep a reference to the staging buffer so it doesn't
        // get erased when we exit the scope
        staging_buffers.push_back(S);
        copy_cmd.copySubBuffer( S, CubeObj.GetIndexBuffer(), vk::BufferCopy{ 0 , 0 , byte_size } );
    }

    copy_cmd.end();
    C.submit_cmd_buffer( copy_cmd );
    cp->FreeCommandBuffer( copy_cmd );

    return CubeObj;
}

int main(int argc, char ** argv)
{
    //==========================================================================
    // 1. Initlize the library and create a GLFW window
    //==========================================================================
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE,  GLFW_FALSE); GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, APP_TITLE, nullptr, nullptr);

    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);


    // the context is the main class for the vka library. It is keeps track of
    // all the vulkan objects and releases them appropriately when it is destroyed
    // It is also responsible for creating the objects such as buffers, textures
    // command pools, etc.
    vka::context C;

    // Enable the required extensions for being able to draw
    for(uint i=0;i<glfwExtensionCount;i++)  C.enable_extension( glfwExtensions[i] );

    // Enable some extra extensions that we want.
    C.enable_extension( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );

    // Enable the required device extension
    C.enable_device_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    C.init();

    vk::SurfaceKHR surface;
    if (glfwCreateWindowSurface( C.get_instance(), window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface) ) != VK_SUCCESS)
    {
        ERROR << "Failed to create window surface!" << ENDL;
        throw std::runtime_error("failed to create window surface!");
    }

    C.create_device(surface); // find the appropriate device

    // The Screen is essentially a wrapper around the Swapchain, a default Renderpass
    // and framebuffers.
    // in VKA we present images to the screen object.
    // This a simple initialization of creating a screen with depth testing
    auto * screen = C.new_screen("screen");
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
    // Initalize all the Buffer/Texture Pools
    //==============================================================================
    vka::BufferMemoryPool StagingBufferPool(&C);
    StagingBufferPool.SetMemoryProperties( vk::MemoryPropertyFlagBits::eHostCoherent| vk::MemoryPropertyFlagBits::eHostVisible);
    StagingBufferPool.SetSize(10*1024*1024);
    StagingBufferPool.SetUsage( vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc| vk::BufferUsageFlagBits::eIndexBuffer| vk::BufferUsageFlagBits::eVertexBuffer| vk::BufferUsageFlagBits::eUniformBuffer);
    StagingBufferPool.Create();

    vka::BufferMemoryPool BufferPool(&C);
    BufferPool.SetMemoryProperties( vk::MemoryPropertyFlagBits::eDeviceLocal);
    BufferPool.SetSize(10*1024*1024);
    BufferPool.SetUsage( vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc| vk::BufferUsageFlagBits::eIndexBuffer| vk::BufferUsageFlagBits::eVertexBuffer| vk::BufferUsageFlagBits::eUniformBuffer);
    BufferPool.Create();

    vka::TextureMemoryPool TexturePool(&C);
    TexturePool.SetSize( 50*1024*1024 );
    TexturePool.SetUsage( vk::ImageUsageFlagBits::eColorAttachment
                 | vk::ImageUsageFlagBits::eSampled
                 | vk::ImageUsageFlagBits::eTransferDst
                 | vk::ImageUsageFlagBits::eTransferSrc);

    //==============================================================================

#if 1
    vka::host_mesh CubeMesh = vka::box_mesh(1,1,1);
    auto CubeObj = HostToGPU( CubeMesh, BufferPool,StagingBufferPool, cp,C);
#else
        //=====================================================================
        // This is a host_mesh, it's a mesh of a Box stored in RAM.
        // we are going to use this to copy build a Box mesh in GPU memory
        //=====================================================================
        vka::host_mesh CubeMesh = vka::box_mesh(1,1,1);
        auto & P = CubeMesh.get_attribute( vka::VertexAttribute::ePosition );
        auto & U = CubeMesh.get_attribute( vka::VertexAttribute::eUV);
        auto & N = CubeMesh.get_attribute( vka::VertexAttribute::eNormal);
        auto & I = CubeMesh.get_attribute( vka::VertexAttribute::eIndex);
        //=====================================================================



        // A MeshObject is essentially just a container that holde
        // information about a Mesh. It contains multiple SubBuffers; one for
        // each attribute in the mesh (eg: position, UV, normals)
        // and an additional optional buffer for Indices.
        //
        // MeshObjects can be bound using a command buffer similarly to how
        // a vertex or index buffer is bound. Under the hood, it simply
        // binds the individual buffers to their appropriate bind index.
        vka::MeshObject CubeObj;

        // allocate a new buffer from teh buffer pool and add it as an attribute
        // to the CubeObject.
        CubeObj.AddAttributeBuffer(0,  BufferPool.NewSubBuffer(P.data_size()) );
        CubeObj.AddAttributeBuffer(1,  BufferPool.NewSubBuffer(U.data_size()) );
        CubeObj.AddAttributeBuffer(2,  BufferPool.NewSubBuffer(N.data_size()) );
        // Also allocate an Index Buffer
        CubeObj.AddIndexBuffer( vk::IndexType::eUint16,  BufferPool.NewSubBuffer(I.data_size()) );



        // We're going to keep a vector of staging buffers so that
        // they do not deallocate themselves until after we are
        // done with them.
        std::vector<vka::SubBuffer_p>    staging_buffers;

        vka::command_buffer copy_cmd = cp->AllocateCommandBuffer();
        copy_cmd.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );


            // First copy the Index buffer by allocating a StagingBuffer from the StagingBufferPool
            // Copy the data from the host to the staging buffer
            // write the command to copy from the staging buffer to the attribute buffer.
            {
                auto byte_size = I.data_size();
                auto S = StagingBufferPool.NewSubBuffer( byte_size );

                // copy data to staging buffer
                S->CopyData( I.data(), byte_size );

                // keep a reference to the staging buffer so it doesn't
                // get erased when we exit the scope
                staging_buffers.push_back(S);
                copy_cmd.copySubBuffer( S, CubeObj.GetIndexBuffer(), vk::BufferCopy{ 0 , 0 , byte_size } );
            }

            // For each Attribute: do the same as we did for the index buffer
            vka::host_mesh::AttributeInfo_t * A[] = {&P, &U, &N};
            for(uint32_t i=0;i<3;i++)
            {
                auto byte_size = A[i]->data_size();

                // Create a staging buffer for copying attribute i;
                auto S = StagingBufferPool.NewSubBuffer( byte_size );

                // copy data to staging buffer
                S->CopyData( A[i]->data(), byte_size );

                // keep a reference to the staging buffer so it doesn't
                // get erased.
                staging_buffers.push_back(S);

                copy_cmd.copySubBuffer( S, CubeObj.GetAttributeBuffer(i), vk::BufferCopy{ 0 , 0 , byte_size } );

            }


        copy_cmd.end();
        C.submit_cmd_buffer( copy_cmd );
        cp->FreeCommandBuffer( copy_cmd );

        // We no longer need the staging buffers, so we can
        // clear this.
        staging_buffers.clear();

#endif
        // Create two buffers, one for vertices and one for indices. THey
        // will each be 1024 bytes long
        auto U_buffer  = BufferPool.NewSubBuffer(5*1024);
//==============================================================================
// Create a texture
//
//==============================================================================

    // 1. First load host_image into memory, and specifcy we want 4 channels.
        vka::host_image D("resources/textures/Brick-2852a.jpg",4);
        vka::host_image D2("resources/textures/noise.jpg",4);

    // 2. Use the context's helper function to create a device local texture
    //    We will be using a texture2d which is a case specific version of the
    //    generic texture
        auto Tex = TexturePool.AllocateTexture2D( vk::Format::eR8G8B8A8Unorm,
                                         vk::Extent2D(D.width(), D.height() ),
                                         2,5
                                         );



        // 3. Create a scope so that when we create Staging Buffers, they'll automatically
        //    be deallocated. StagingBuffers allocated from a pool can be allocated and deallocated
        //    without much performance issues.
        {
            auto StagingBuffer = StagingBufferPool.NewSubBuffer( D.size() + D2.size() );
            StagingBuffer->CopyData( D.data(), D.size() );
            StagingBuffer->CopyData(D2.data(), D2.size() , D.size());


        // 4. Now that the data is on the device. We need to get it from the buffer
        //    to the texture. To do this we will record a command buffer to do the
        //    following:
        //         a. convert the texture2d into a layout which can accept transfer data
        //         b. copy the data from the buffer to the texture2d.
        //         c. convert the texture2d into a layout which is good for shader use

            // allocate the command buffer
            vka::command_buffer cb1 = cp->AllocateCommandBuffer();
            cb1.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );

            // a. convert the texture to eTransferDstOptimal
            cb1.convertTextureLayerMips( Tex,
                                         0,2, // convert Layer's 0-1
                                         0,1, // convert mip level i
                                         vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                                         vk::PipelineStageFlagBits::eAllCommands,
                                         vk::PipelineStageFlagBits::eAllCommands);

            // b. copy the data from the buffer to the texture
            vk::BufferImageCopy BIC;
            BIC.setBufferImageHeight(  D.height() )
               .setBufferOffset(0) // the image data starts at the start of the SubBuffer
               .setImageExtent( vk::Extent3D(D.width(), D.height(), 1) ) // size of the image
               .setImageOffset( vk::Offset3D(0,0,0)) // where in the texture we want to paste the image
               .imageSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
                                .setBaseArrayLayer(0) // the layer to copy
                                .setLayerCount(2) // only copy one layer
                                .setMipLevel(0);  // only the first mip-map level


            //---------------------------------------------
            cb1.copySubBufferToTexture( StagingBuffer, Tex, vk::ImageLayout::eTransferDstOptimal, BIC);


            cb1.generateMipMaps( Tex, 0, 2);

            cb1.end();
            C.submit_cmd_buffer(cb1);
            // free the command buffer
            cp->FreeCommandBuffer(cb1);
        }
//==============================================================================


//==============================================================================
// Create a Rendering pipeline
//
//==============================================================================
        vka::pipeline* pipeline = C.new_pipeline("triangle");

        // Create the graphics Pipeline
          pipeline->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                  ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

                  ->set_vertex_shader(   "resources/shaders/push_consts_default/push_consts_default.vert" , "main")   // the shaders we want to use
                  ->set_fragment_shader( "resources/shaders/push_consts_default/push_consts_default.frag" , "main") // the shaders we want to use

                  // tell the pipeline that attribute 0 contains 3 floats
                  // and the data starts at offset 0
                  ->set_vertex_attribute(0, 0,  0 , vk::Format::eR32G32B32Sfloat, sizeof(glm::vec3) )
                  // tell the pipeline that attribute 1 contains 3 floats
                  // and the data starts at offset 12
                  ->set_vertex_attribute(1, 1,  0 , vk::Format::eR32G32Sfloat , sizeof(glm::vec2) )

                  ->set_vertex_attribute(2, 2,  0 , vk::Format::eR32G32B32Sfloat , sizeof(glm::vec3) )


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
    texture_descriptor->AttachSampler(0, Tex);
    texture_descriptor->update();

    vka::descriptor_set * ubuffer_descriptor = pipeline->create_new_descriptor_set(1, descriptor_pool);
    ubuffer_descriptor->AttachUniformBuffer(0,U_buffer, 10);
    ubuffer_descriptor->update();

    // We will allocate two Staging buffers to copy uniform data as well as dynamic uniform data
    // for each of the objects. Each of the Staging Buffers act like an individual buffer
    // But are simply an offset into the BufferPool it was allocated from.
    //
    //
    // +--------------+
    // | uniform_data |  UniformStagingBuffer
    // +--------------+
    //
    // +------+------+
    // | obj1 | obj2 |    DynamicUniformStagingBuffer
    // +------+------+
    //
    // +--------------+---------+------+------+-------------------------+
    // | uniform_data |         | obj1 | obj2 |                         | StagingBufferPool
    // +--------------+---------+------+------+-------------------------+
    auto UniformStagingBuffer        = StagingBufferPool.NewSubBuffer(   sizeof(uniform_buffer_t ));

    // Get a MappedMemory object so that we can write data directly into it.
    vka::MappedMemory  UniformStagingBufferMap = UniformStagingBuffer->GetMappedMemory();

    // Cast the memory to a reference so we can access
    // aliased data.
    uniform_buffer_t & UniformStagingStruct               = *( (uniform_buffer_t*)UniformStagingBufferMap );

    vka::command_buffer cb = cp->AllocateCommandBuffer();


    vka::semaphore * image_available_semaphore = C.new_semaphore("image_available_semaphore");
    vka::semaphore * render_complete_semaphore = C.new_semaphore("render_complete_semaphore");



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


      //--------------------------------------------------------------------------------------
      // Copy the Data from thost to the staging buffers.
      //--------------------------------------------------------------------------------------
      #define MAX_OBJECTS 2
      // Copy the uniform buffer data into the staging buffer
      const float AR = WIDTH / ( float )HEIGHT;
      UniformStagingStruct.view        = glm::lookAt( glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
      UniformStagingStruct.proj        = glm::perspective(glm::radians(45.0f), AR, 0.1f, 30.0f);
      UniformStagingStruct.proj[1][1] *= -1;
      //--------------------------------------------------------------------------------------

      //--------------------------------------------------------------------------------------
      // Copy the uniform buffer data from the staging buffer to the uniform buffer. This normally only needs to be done
      // once per rendering frame because it contains frame constant data.
      cb.copySubBuffer( UniformStagingBuffer ,  U_buffer , vk::BufferCopy{ 0,0, sizeof(uniform_buffer_t) } );
      //--------------------------------------------------------------------------------------

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

        // Bind the MeshObject. That is, bind each of the buffers in the mesh object
        // to their appropriate index.
        cb.bindMeshObject( CubeObj );

      //========================================================================
      // Draw all the objects while binding the dynamic uniform buffer
      // to
      //========================================================================
      for(uint32_t j=0 ; j < MAX_OBJECTS; j++)
      {
            // Here we write the data to the command buffer.
            push_constants_t push;
            push.index    = j%2;

            if(j==0)
            {
              push.miplevel = -1;
            } else {
              push.miplevel = (int)fmod( t ,Tex->GetMipLevels() );
            }

            float x = j%2 ? -1 : 1;
            push.model = glm::rotate(glm::mat4(1.0), t * glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f )) * glm::translate( glm::mat4(), glm::vec3(x,0,0) );
            cb.pushConstants( pipeline->get_layout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(push_constants_t), &push);

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
      screen->present_frame(frame_index, render_complete_semaphore);

      std::this_thread::sleep_for( std::chrono::milliseconds(3) );
    }

    return 0;
}

#define STB_IMAGE_IMPLEMENTATION
#include<stb/stb_image.h>
