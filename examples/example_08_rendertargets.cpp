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
#include <vka/vka.h>

#include <vka/core/primatives.h>

#include <vka/core2/MeshObject.h>
#include <vka/core2/RenderTarget2.h>
#include <vka/math/linalg.h>

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

struct compose_pipeline_push_consts
{
    glm::vec2 position;
    glm::vec2 size;
    int layer;
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
                           vka::CommandPool & CP,
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


    vka::command_buffer copy_cmd = CP.AllocateCommandBuffer();
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
    CP.FreeCommandBuffer( copy_cmd );

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
    vka::Screen Screen(&C);
    Screen.Create(surface, vk::Extent2D(WIDTH,HEIGHT));
    //==========================================================================





    //==========================================================================
    // Initialize the Command and Descriptor Pools
    //==========================================================================
    vka::DescriptorPool descriptor_pool(&C);
    descriptor_pool.set_pool_size(vk::DescriptorType::eCombinedImageSampler, 10);
    descriptor_pool.set_pool_size(vk::DescriptorType::eUniformBuffer, 1);
    // [NEW]
    descriptor_pool.set_pool_size(vk::DescriptorType::eUniformBufferDynamic, 1);

    descriptor_pool.create();

    vka::CommandPool CP(&C);
    CP.create();
    //==========================================================================


    vka::RenderTarget2 myRenderTarget(&C);
    myRenderTarget.SetExtent( vk::Extent2D(1024,768));
    myRenderTarget.Create( { vk::Format::eR32G32B32A32Sfloat,
                             vk::Format::eR32G32B32A32Sfloat,
                             vk::Format::eR8G8B8A8Unorm },  vk::Format::eD32Sfloat);


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
    TexturePool.SetTiling( vk::ImageTiling::eOptimal );
    TexturePool.SetUsage(
                  vk::ImageUsageFlagBits::eSampled
                 | vk::ImageUsageFlagBits::eTransferDst
                 | vk::ImageUsageFlagBits::eTransferSrc);


    vka::host_mesh CubeMesh = vka::box_mesh(1,1,1);
    auto CubeObj = HostToGPU( CubeMesh, BufferPool,StagingBufferPool, CP, C);

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
            vka::command_buffer cb1 = CP.AllocateCommandBuffer();
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
            CP.FreeCommandBuffer(cb1);
        }
//==============================================================================


//==============================================================================
// Create a Rendering pipeline
//
//==============================================================================

        vka::Pipeline g_buffer_pipeline(&C);
        g_buffer_pipeline.setViewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                ->setScissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

                ->setVertexShader(   "resources/shaders/gbuffer/gbuffer.vert", "main" )   // the shaders we want to use
                ->setFragmentShader( "resources/shaders/gbuffer/gbuffer.frag", "main" ) // the shaders we want to use

                // tell the pipeline that attribute 0 contains 3 floats
                // and the data starts at offset 0
                ->setVertexAttribute(0, 0 ,  0 , vk::Format::eR32G32B32Sfloat , sizeof(glm::vec3) )
                // tell the pipeline that attribute 1 contains 3 floats
                // and the data starts at offset 12
                ->setVertexAttribute(1, 1 ,  0 , vk::Format::eR32G32Sfloat , sizeof(glm::vec2) )

                ->setVertexAttribute(2, 2 ,  0 , vk::Format::eR32G32B32Sfloat , sizeof(glm::vec3) )

                //===============================================================
                // Here we are going to set the number of color attachments.
                // We created 3 color attachments for our render target.
                //===============================================================
                ->setColorAttachments( 3 )
                //===============================================================

                // Triangle vertices are drawn in a counter clockwise manner
                // using the right hand rule which indicates which face is the
                // front
                ->setFrontFace(vk::FrontFace::eCounterClockwise)

                // Cull all back facing triangles.
                ->setCullMode(vk::CullModeFlagBits::eBack)

                // Tell the shader that we are going to use a texture
                // in Set #0 binding #0
                ->addTextureLayoutBinding(0, 0, vk::ShaderStageFlagBits::eFragment)

                // Tell teh shader that we are going to use a uniform buffer
                // in Set #0 binding #0
                ->addUniformLayoutBinding(1, 0, vk::ShaderStageFlagBits::eVertex)

                // Add a push constant to the layout. It is accessable in the vertex shader
                // stage only.
                ->addPushConstant( sizeof(push_constants_t), 0, vk::ShaderStageFlagBits::eVertex)
                //
                //===============================================================
                // Since we are no longer drawing to the main screen. we need
                // to set the render pass to the OffscreenTarget
                ->setRenderPass(  myRenderTarget.GetRenderPass());
                //===============================================================
        g_buffer_pipeline.getColorBlendAttachmentState(0).blendEnable=VK_FALSE;
        g_buffer_pipeline.getColorBlendAttachmentState(1).blendEnable=VK_FALSE;
        g_buffer_pipeline.getColorBlendAttachmentState(2).blendEnable=VK_FALSE;
        g_buffer_pipeline.create();


        //======================================================================
        // Now we will generate a new pipeline which will draw a fullscreen
        // quad on the screen. It will take in the 4 texture images we
        // created in the rendertarget and compose them into a single image
        // which can be displayed to the screen
        //======================================================================
        vka::Pipeline compose_pipeline(&C);
        compose_pipeline.setViewport(vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                ->setScissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

                ->setVertexShader(   "resources/shaders/compose_rendertargets/compose_rendertargets.vert", "main" )   // the shaders we want to use
                ->setFragmentShader( "resources/shaders/compose_rendertargets/compose_rendertargets.frag", "main" ) // the shaders we want to use

                ->setTopology(vk::PrimitiveTopology::eTriangleList)
                ->setLineWidth(1.0f)
                // Triangle vertices are drawn in a counter clockwise manner
                // using the right hand rule which indicates which face is the
                // front
                ->setFrontFace(vk::FrontFace::eCounterClockwise)

                // Here are the 4 textures we are going to need
                // See the fragment shader code for more information.
                ->addTextureLayoutBinding(0, 0, vk::ShaderStageFlagBits::eFragment)
                ->addTextureLayoutBinding(0, 1, vk::ShaderStageFlagBits::eFragment)
                ->addTextureLayoutBinding(0, 2, vk::ShaderStageFlagBits::eFragment)
                ->addTextureLayoutBinding(0, 3, vk::ShaderStageFlagBits::eFragment)

                // Cull all back facing triangles.
                ->setCullMode(vk::CullModeFlagBits::eNone)

                // Add a push constant to the layout. It is accessable in the vertex shader
                // stage only.
                ->addPushConstant( sizeof(compose_pipeline_push_consts), 0, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
                // Since we are drawing this to the screen, we need the screen's
                // renderpass.
                ->setRenderPass( Screen.GetRenderPass() )
                ->create();
        //======================================================================

        //======================================================================
        // Each attachment in the render target is an image. Once we draw to the
        // image, we can essentially read from it like it was any other texture.
        // Now we are goign to take those images and create a descriptor set for
        // it.

        // Create a new descriptor set based on the descriptor information we
        // gave to the Compose pipeline
        vka::DescriptorSet_p  renderTargets = compose_pipeline.createNewDescriptorSet(0, &descriptor_pool);
        renderTargets->AttachSampler(0, myRenderTarget.GetColorImage(0) );
        renderTargets->AttachSampler(1, myRenderTarget.GetColorImage(1) );
        renderTargets->AttachSampler(2, myRenderTarget.GetColorImage(2) );
        renderTargets->AttachSampler(3, myRenderTarget.GetDepthImage() );
        renderTargets->update();
        //======================================================================



//==============================================================================
// Create a descriptor set:
//   once the pipeline has been created. We need to create a descriptor set
//   which we can use to tell what textures we want to use in the shader.
//   The pipline object can generate a descriptor set for you.
//==============================================================================
    // we want a descriptor set for set #0 in the pipeline.
    vka::DescriptorSet_p  texture_descriptor = g_buffer_pipeline.createNewDescriptorSet(0, &descriptor_pool);
    //  attach our texture to binding 0 in the set.
    texture_descriptor->AttachSampler(0, Tex);
    texture_descriptor->update();

    vka::DescriptorSet_p  ubuffer_descriptor = g_buffer_pipeline.createNewDescriptorSet(1, &descriptor_pool);
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

    //vka::command_buffer cb = CP.AllocateCommandBuffer();
    vka::command_buffer offscreen_cmd_buffer = CP.AllocateCommandBuffer();
    vka::command_buffer compose_cmd_buffer = CP.AllocateCommandBuffer();

    vka::Semaphore_p  image_available_semaphore  = C.createSemaphore();
    vka::Semaphore_p  gbuffer_complete_semaphore = C.createSemaphore();
    vka::Semaphore_p  render_complete_semaphore  = C.createSemaphore();



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
      offscreen_cmd_buffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
      offscreen_cmd_buffer.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse) );


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
          offscreen_cmd_buffer.copySubBuffer( UniformStagingBuffer ,  U_buffer , vk::BufferCopy{ 0,0, sizeof(uniform_buffer_t) } );
          //--------------------------------------------------------------------------------------

          //--------------------------------------------------------------------------------------
          // Draw to the Render Target
          //--------------------------------------------------------------------------------------
          offscreen_cmd_buffer.beginRender( myRenderTarget );
          {
              offscreen_cmd_buffer.bindPipeline( vk::PipelineBindPoint::eGraphics, g_buffer_pipeline);

              offscreen_cmd_buffer.bindDescriptorSets( vk::PipelineBindPoint::eGraphics,
                                     g_buffer_pipeline.getLayout(),
                                     0,
                                     vk::ArrayProxy<const vk::DescriptorSet>( texture_descriptor->get()),
                                     nullptr );

              offscreen_cmd_buffer.bindDescriptorSets( vk::PipelineBindPoint::eGraphics,
                                     g_buffer_pipeline.getLayout(),
                                     1,
                                     vk::ArrayProxy<const vk::DescriptorSet>( ubuffer_descriptor->get()),
                                     nullptr );


              offscreen_cmd_buffer.bindMeshObject( CubeObj );

              for(uint32_t j=0 ; j < MAX_OBJECTS; j++ )
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
                    offscreen_cmd_buffer.pushConstants( g_buffer_pipeline.getLayout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(push_constants_t), &push);

                    // draw 3 indices, 1 time, starting from index 0, using a vertex offset of 0
                    offscreen_cmd_buffer.drawIndexed(36, 1, 0 , 0, 0);
              }
          }
          offscreen_cmd_buffer.endRenderPass();


      offscreen_cmd_buffer.end();
      //--------------------------------------------------------------------------------------

      //========================================================================
      //========================================================================
      //========================================================================
      //========================================================================

      // Now we are going to build the command buffer to render to the screen
      // We are going to draw 4 quads onto the screen, each quad will
      // use a different color component from the previous rendering stage
      // as a texture so we can see what image is drawn to each of the components.


      // Prepare the next frame for rendering and get the frame_index
      // that we will be drawing to.
      //    - The semaphore we pass into it will be signaled when the frame is
      //      ready
      uint32_t frame_index = Screen.GetNextFrameIndex(image_available_semaphore);

      compose_cmd_buffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
      compose_cmd_buffer.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse) );
      {
          // start the actual rendering
          compose_cmd_buffer.beginRender(Screen, frame_index);
          {
              compose_cmd_buffer.bindPipeline( vk::PipelineBindPoint::eGraphics, compose_pipeline );

              compose_cmd_buffer.bindDescriptorSet(vk::PipelineBindPoint::eGraphics,
                                   compose_pipeline,
                                   0, // binding index
                                   renderTargets);

              //compose_pipeline_push_consts pc;

              std::array<compose_pipeline_push_consts, 4> Push_Consts =
              {
                  compose_pipeline_push_consts{ glm::vec2(-1,-1), glm::vec2(1,1), 0 },
                  compose_pipeline_push_consts{ glm::vec2( 0, 0), glm::vec2(1,1), 1 },
                  compose_pipeline_push_consts{ glm::vec2( 0,-1), glm::vec2(1,1), 2 },
                  compose_pipeline_push_consts{ glm::vec2(-1, 0), glm::vec2(1,1), 3 }
              };

              for(auto & pc : Push_Consts)
              {

                  compose_cmd_buffer.pushConstants( compose_pipeline.getLayout(),
                                                 vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                                                 0,
                                                 sizeof(compose_pipeline_push_consts),
                                                 &pc);
                  compose_cmd_buffer.draw(6,1,0,0);
              }
          }
          compose_cmd_buffer.endRenderPass();
      }
      compose_cmd_buffer.end();

      //========================================================================

      // Submit the command buffers, but wait until the image_available_semaphore
      // is flagged. Once the commands have been executed, flag the render_complete_semaphore
      C.submit_command_buffer( offscreen_cmd_buffer,        // submit this command buffer
                               image_available_semaphore,   // // Wait for this semaphore before we start writing
                               gbuffer_complete_semaphore); // Signal this semaphore when the commands have been executed


      C.submit_command_buffer( compose_cmd_buffer,        // Execute this command buffer
                               gbuffer_complete_semaphore, // but wait until this semaphore has been signaled
                               render_complete_semaphore);// signal this semaphore when it is done


      // present the image to the surface, but wait for the render_complete_semaphore
      // to be flagged by the submit_command_buffer
      Screen.PresentFrame(frame_index, render_complete_semaphore);

      std::this_thread::sleep_for( std::chrono::milliseconds(3) );
    }

    return 0;
}

#define STB_IMAGE_IMPLEMENTATION
#include<stb/stb_image.h>
