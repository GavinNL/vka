/*
 * Example 6: Camera,  Transformation, and Window Inputs
 *
 * This example demonstrates the use of three classes: vka::camera,
 * vka::transform and vka::GLFW_Window_Handler.
 *
 * vka::transform is a class which makes manipulating the postion/orientation
 * of an object easier.
 *
 * The vka::camera class assists in creating the matrices required for
 * the view and projection matrices. In addition it also provides some basic
 * physics to provide motion.
 *
 * call camera.calculate() in the main loop to do the physics calcations
 *
 * vka::GLFW_Window_Handler is a wrapper a around the GLFWWindow, it provides
 * callbacks for input events such as key presses and mouse motion.
 *
 * In this example we are going to use all three of these classes.  In addition
 * we are going to take Example_05 and remove all the verbose methods and replace
 * them with vka specific helper functions.
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



#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#define WIDTH 1024
#define HEIGHT 768
#define APP_TITLE "Example_06 - Camera Control , Transformations, and Sub Buffers"

#include <vka/core/camera.h>
#include <vka/core/transform.h>
#include <vka/core/primatives.h>

#include <vka/utils/buffer_memory_manager.h>
#include <vka/core/managed_buffer.h>
#include <vka/utils/buffer_pool.h>

#include <vka/utils/glfw_window_handler.h>

#include "vulkan_app.h"

// This is the structure of the uniform buffer we want.
// it needs to match the structure in the shader.
struct uniform_buffer_t
{
    glm::mat4 view;
    glm::mat4 proj;
};

struct per_frame_uniform_t
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
 * @brief The mesh_info_t struct
 * Information about the mesh, number of indices to draw and
 * offset in the index array
 */
struct mesh_info_t
{
    uint32_t index_offset; // index offset
    uint32_t count; // number of indices or vertices
    uint32_t vertex_offset; // vertex offset
};


/**
 * @brief The Object_t struct
 *
 * This class holds all information about an object we want to draw.
 */
struct Object_t
{
    vka::transform           m_transform; // the world space transformation


    vka::pipeline           *m_pipeline; // the graphcis pipeline to use

    dynamic_uniform_buffer_t m_uniform;  // the struct of the uniform buffer

    size_t                   m_uniform_offset; // the offset into the buffer

    push_constants_t         m_push; // push_constants

    mesh_info_t              m_mesh; // the mesh to use
};

class PhysicsComponent_t
{
    vka::transform m_transform;
};

class RenderComponent_t
{
public:
    vka::pipeline   *m_pipeline = nullptr;
    vka::sub_buffer *m_ibuffer  = nullptr;
    vka::sub_buffer *m_vbuffer  = nullptr;

    vka::descriptor_set * m_texture        = nullptr;
    vka::descriptor_set * m_uniform_buffer = nullptr;
    vka::descriptor_set * m_dynamic_buffer = nullptr;

    vk::DeviceSize        m_dynamic_offset = 0;

    push_constants_t         m_push;

    dynamic_uniform_buffer_t m_uniform_data;
    mesh_info_t              m_mesh;
};



class ComponentRenderer_t
{
    vka::pipeline * m_pipeline = nullptr;
    vka::sub_buffer *m_ibuffer = nullptr;
    vka::sub_buffer *m_vbuffer = nullptr;

public:
    // given a render component, draw
    // it into the command buffer.
    void operator ()( vka::command_buffer & cb,
                      RenderComponent_t * obj)
    {
        // ===== bind the pipeline that we want to use next =======
        if( obj->m_pipeline != m_pipeline)
        {
            m_pipeline = obj->m_pipeline;
            m_vbuffer = obj->m_vbuffer;
            m_ibuffer = obj->m_ibuffer;
            cb.bindVertexSubBuffer(0, m_vbuffer);
            cb.bindIndexSubBuffer(m_ibuffer, vk::IndexType::eUint16);
            cb.bindPipeline( vk::PipelineBindPoint::eGraphics, *m_pipeline );
            cb.bindDescriptorSet(vk::PipelineBindPoint::eGraphics,
                                 m_pipeline,
                                 0, // binding index
                                 obj->m_texture);

            cb.bindDescriptorSet(vk::PipelineBindPoint::eGraphics,
                                 m_pipeline,
                                 1, // binding index
                                 obj->m_uniform_buffer);

            cb.bindDescriptorSet(vk::PipelineBindPoint::eGraphics,
                                 m_pipeline,
                                 2, // binding index
                                 obj->m_dynamic_buffer,
                                 obj->m_dynamic_offset);
        }


        obj->m_push.index    =   0;
        obj->m_push.miplevel =  -1;


        cb.pushConstants( m_pipeline->get_layout(),
                          vk::ShaderStageFlagBits::eVertex,
                          0,
                          sizeof(push_constants_t),
                          &obj->m_push);

        mesh_info_t const & m = obj->m_mesh;
        cb.drawIndexed(m.count, 1 , m.index_offset, m.vertex_offset, 0);
    }
};



struct Entity_t
{
    PhysicsComponent_t *m_physics = nullptr;
    RenderComponent_t  *m_render  = nullptr;
};





struct App : public VulkanApp
{

  void init_pools()
  {
      //==========================================================================
      // Initialize the Command and Descriptor Pools
      //==========================================================================
      m_descriptor_pool = m_Context.new_descriptor_pool("main_desc_pool");
      m_descriptor_pool->set_pool_size(vk::DescriptorType::eCombinedImageSampler, 2);
      m_descriptor_pool->set_pool_size(vk::DescriptorType::eUniformBuffer, 1);
      m_descriptor_pool->set_pool_size(vk::DescriptorType::eUniformBufferDynamic, 1);
      m_descriptor_pool->create();

      m_command_pool = m_Context.new_command_pool("main_command_pool");
      //==========================================================================
  }


  void init_memory()
  {
      m_buffer_pool = m_Context.new_buffer_pool("main_buffer_pool");
      m_buffer_pool->set_size( 50*1024*1024 ); // create 50Mb buffer
      m_buffer_pool->create();

      // Allocate sub buffers from the buffer pool. These buffers can be
      // used for indices/vertices or uniforms.
      m_dbuffer = m_buffer_pool->new_buffer( 10*1024*1024 );
      m_ubuffer = m_buffer_pool->new_buffer( 10*1024*1024 );
      m_vbuffer = m_buffer_pool->new_buffer( 10*1024*1024 );
      m_ibuffer = m_buffer_pool->new_buffer( 10*1024*1024 );
      m_sbuffer = m_Context.new_staging_buffer( "staging_buffer", 10*1024*1024);

      m_texture_array = m_Context.new_texture2darray("main_texture_array");
      m_texture_array->set_size( 512 , 512 );
      m_texture_array->set_format(vk::Format::eR8G8B8A8Unorm);
      m_texture_array->set_mipmap_levels(8);
      m_texture_array->set_layers(10);
      m_texture_array->create();
      m_texture_array->create_image_view(vk::ImageAspectFlagBits::eColor);
  }

  void init_pipelines()
  {
      // Create the graphics Pipeline
      auto M = vka::box_mesh(1,1,1);

        m_pipelines.main = m_Context.new_pipeline("main_pipeline");
        m_pipelines.main->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

                ->set_vertex_shader(   "resources/shaders/mipmaps/mipmaps_v.spv", "main" )   // the shaders we want to use
                ->set_fragment_shader( "resources/shaders/mipmaps/mipmaps_f.spv", "main" ) // the shaders we want to use

                // tell the pipeline that attribute 0 contains 3 floats
                // and the data starts at offset 0
                ->set_vertex_attribute(0 ,  M.offset( vka::VertexAttribute::ePosition) , M.format( vka::VertexAttribute::ePosition) , M.vertex_size() )
                // tell the pipeline that attribute 1 contains 3 floats
                // and the data starts at offset 12
                ->set_vertex_attribute(1 ,  M.offset( vka::VertexAttribute::eUV)       , M.format( vka::VertexAttribute::eUV) , M.vertex_size() )

                ->set_vertex_attribute(2 ,  M.offset( vka::VertexAttribute::eNormal)   , M.format( vka::VertexAttribute::eNormal) , M.vertex_size() )

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

                // Add a push constant to the layout. It is accessable in the vertex shader
                // stage only.
                ->add_push_constant( sizeof(push_constants_t), 0, vk::ShaderStageFlagBits::eVertex)
                //
                ->set_render_pass( m_default_renderpass )
                ->create();



          m_pipelines.line = m_Context.new_pipeline("line_pipeline");
          m_pipelines.line->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                  ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

                  ->set_vertex_shader(   "resources/shaders/line_shader/line_shader_v.spv" , "main") // the shaders we want to use
                  ->set_fragment_shader( "resources/shaders/line_shader/line_shader_f.spv" , "main") // the shaders we want to use

                  // tell the pipeline that attribute 0 contains 3 floats
                  // and the data starts at offset 0
                  ->set_vertex_attribute(0 ,  0 ,  vk::Format::eR32G32B32Sfloat, 7*sizeof(float) )
                  // tell the pipeline that attribute 1 contains 3 floats
                  // and the data starts at offset 12
                  ->set_vertex_attribute(1 ,  3*sizeof(float)       , vk::Format::eR32G32B32A32Sfloat , 7*sizeof(float) )

                  ->set_toplogy( vk::PrimitiveTopology::eLineList)
                  // Triangle vertices are drawn in a counter clockwise manner
                  // using the right hand rule which indicates which face is the
                  // front
                  ->set_front_face(vk::FrontFace::eCounterClockwise)

                  // Cull all back facing triangles.
                  ->set_cull_mode(vk::CullModeFlagBits::eBack)

                  // Tell teh shader that we are going to use a uniform buffer
                  // in Set #0 binding #0
                  ->add_uniform_layout_binding(0, 0, vk::ShaderStageFlagBits::eVertex)

                  // Tell teh shader that we are going to use a uniform buffer
                  // in Set #0 binding #0
                  ->add_dynamic_uniform_layout_binding(1, 0, vk::ShaderStageFlagBits::eVertex)

                  //
                  ->set_render_pass( m_default_renderpass )
                  ->create();
  }


  void load_meshs()
  {
      std::vector<vka::mesh_t>   meshs;
      meshs.push_back( vka::box_mesh(1,1,1) );
      meshs.push_back( vka::sphere_mesh(0.5,20,20) );

      for( auto & M : meshs)
      {

          // sub_buffer::insert inserts data into the sub buffer at an available
          // space and returns a sub_buffer object.

          auto m1v = m_vbuffer->insert( M.vertex_data(), M.vertex_data_size() );
          auto m1i = m_ibuffer->insert(  M.index_data() , M.index_data_size()  );

          // If you wish to free the memory you have allocated, you shoudl call
          // vertex_buffer->free_buffer_object(m1v);

          assert( m1v.m_size != 0);
          assert( m1i.m_size != 0);

          mesh_info_t mesh;
          mesh.count         = M.num_indices();

          // the offset returned is the byte offset, so we need to divide it
          // by the index/vertex size to get the actual index/vertex offset
          mesh.index_offset  =  m1i.m_offset  / M.index_size();
          mesh.vertex_offset =  m1v.m_offset  / M.vertex_size();

          m_mesh_info.push_back(mesh);
      }
  }

  void load_textures()
  {
      // 1. First load host_image into memory, and specifcy we want 4 channels.
          std::vector<std::string> image_paths = {
              "resources/textures/Brick-2852a.jpg",
              "resources/textures/noise.jpg"
          };

          uint32_t layer = 0;
          for(auto & img : image_paths)
          {
              vka::host_image D(img, 4);
              assert( D.width() == 512 && D.height()==512);
              m_texture_array->copy_image( D , layer++, vk::Offset2D(0,0) );
          }

  }



  void init_descriptor_sets()
  {
      // we want a descriptor set for set #0 in the pipeline.
      m_dsets.texture_array = m_pipelines.main->create_new_descriptor_set(0, m_descriptor_pool);
      //  attach our texture to binding 0 in the set.
      m_dsets.texture_array->attach_sampler(0, m_texture_array);
      m_dsets.texture_array->update();

      m_dsets.uniform_buffer = m_pipelines.main->create_new_descriptor_set(1, m_descriptor_pool);
      m_dsets.uniform_buffer->attach_uniform_buffer(0, m_ubuffer, sizeof(uniform_buffer_t), m_ubuffer->offset());
      m_dsets.uniform_buffer->update();


      m_dsets.dynamic_uniform_buffer = m_pipelines.main->create_new_descriptor_set(2, m_descriptor_pool);
      m_dsets.dynamic_uniform_buffer->attach_dynamic_uniform_buffer(0, m_dbuffer, sizeof(dynamic_uniform_buffer_t), m_dbuffer->offset());
      m_dsets.dynamic_uniform_buffer->update();

  }
  virtual void onInit()
  {
      init_pools();
      init_memory();

      load_meshs();
      load_textures();


      init_pipelines();
      init_descriptor_sets();


      m_image_available_semaphore = m_Context.new_semaphore("image_available_semaphore");
      m_render_complete_semaphore = m_Context.new_semaphore("render_complete_semaphore");

      m_command_buffer = m_command_pool->AllocateCommandBuffer();

      init_scene();

      keyslot = onKey << [&] (vka::Key k, bool down)
      {
          float x=0;
          float y=0;

          if( is_pressed(vka::Key::A    ) || is_pressed(vka::Key::LEFT )) x += -1;
          if( is_pressed(vka::Key::D    ) || is_pressed(vka::Key::RIGHT)) x +=  1;
          if( is_pressed(vka::Key::W    ) || is_pressed(vka::Key::UP   ))   y += -1;
          if( is_pressed(vka::Key::S    ) || is_pressed(vka::Key::DOWN ))  y +=  1;

          m_Camera.set_acceleration( glm::vec3( x, 0, y ) );

      };

       mouseslot =  onMouseMove << [&] (double dx, double dy)
      {
        dx = mouse_x() - dx;
        dy = mouse_y() - dy;
        if( is_pressed( vka::Button::RIGHT))
        {
            show_cursor(false);
            if( fabs(dx) < 10) m_Camera.yaw(   -dx*0.001f);
            if( fabs(dy) < 10) m_Camera.pitch( dy*0.001f);
        }
        else
        {
            show_cursor(true);
        }
        //std::cout << "Moving: " << dx << ", " << dy << std::endl;
      };

  }

  vka::signal<void(double       , double)>::slot mouseslot;
  vka::signal<void(vka::Key,      int   )>::slot keyslot;


  void UpdateDynamicUniforms(vka::command_buffer & cb)
  {
#define SINGLE_COPY
    auto alignment = 256;

    auto S = static_cast<unsigned char*>(m_sbuffer->map_memory());
    vk::DeviceSize src_offset  = 0;
    vk::DeviceSize dst_offset  = 0;

    vk::DeviceSize size = sizeof( per_frame_uniform_t );
    memcpy( &S[src_offset], &m_frame_uniform, size );


    cb.copySubBuffer( *m_sbuffer, m_ubuffer, vk::BufferCopy(src_offset, 0 ,size)  );
    src_offset += size;

#if defined SINGLE_COPY
    std::vector< vk::BufferCopy > regions;
    regions.reserve(m_Objs.size() );
#endif
    for(auto * comps : m_Objs)
    {
        size = sizeof(comps->m_uniform_data);
        memcpy( &S[src_offset], &comps->m_uniform_data, size );

        auto i = size/alignment + 1;
        if(size%alignment == 0) i--;

#if defined SINGLE_COPY
        regions.push_back( vk::BufferCopy(src_offset , dst_offset + m_dbuffer->offset(), size) ); // must add the offset of the dynamic buffer
#else
        cb.copySubBuffer( *m_sbuffer, m_dbuffer, vk::BufferCopy(src_offset , dst_offset, size) ); // no need for extra offset
#endif

        comps->m_dynamic_offset = dst_offset;

        dst_offset += alignment * i;
        src_offset += size;
    }

#if defined SINGLE_COPY
    cb.copyBuffer( *m_sbuffer, *m_dbuffer, regions);
#endif

  }

  uint32_t UpdateCommandBuffer(vka::command_buffer & cb)
  {
    uint32_t fb_index = m_Context.get_next_image_index(m_image_available_semaphore);

    //  BEGIN RENDER PASS=====================================================
    // We want the to use the render pass we created earlier
    vk::RenderPassBeginInfo renderPassInfo;
    renderPassInfo.renderPass        = *m_default_renderpass;
    renderPassInfo.framebuffer       = *m_framebuffers[fb_index];
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

    // ===== bind the pipeline that we want to use next =======
    ComponentRenderer_t R;
    for(auto * comp : m_Objs)
    {
        R(cb, comp);
    }

    cb.endRenderPass();
    //==============
    return fb_index;


  }


  virtual void onFrame( double dt, double T )
  {


    m_Camera.calculate();
    //  uint32_t fb_index = m_Context.get_next_image_index(m_image_available_semaphore);

    m_frame_uniform.view = m_Camera.get_view_matrix();
    m_frame_uniform.proj = m_Camera.get_proj_matrix();
    m_frame_uniform.proj[1][1] *= -1;


    m_command_buffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
    m_command_buffer.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse) );

        // update uniforms

    auto s1 = microseconds();
        UpdateDynamicUniforms(m_command_buffer);
    auto s2 = microseconds();

        // Render
        auto fb_index = UpdateCommandBuffer(m_command_buffer);
    auto s3 = microseconds();
    m_command_buffer.end();

    m_Context.submit_command_buffer(m_command_buffer, m_image_available_semaphore, m_render_complete_semaphore);
    auto s4 = microseconds();

    m_Context.present_image( fb_index , m_render_complete_semaphore);
    std::cout <<  s2-s1 << "   " <<  s3-s2 << "   "<<  s4-s3 << "   "<< std::endl;
  }



  void init_scene()
  {
      #define MAX_OBJ 1700

      float x = 0;
      float y = 0;
      float z = 0;
      for(int i=0; i<MAX_OBJ ;i++)
      {
        auto * obj = new RenderComponent_t();

        obj->m_ibuffer  = m_ibuffer;
        obj->m_vbuffer  = m_vbuffer;
        obj->m_mesh     = m_mesh_info[0];
        obj->m_pipeline = m_pipelines.main;

        obj->m_uniform_buffer = m_dsets.uniform_buffer;
        obj->m_dynamic_buffer = m_dsets.dynamic_uniform_buffer;
        obj->m_texture        = m_dsets.texture_array;

        m_Objs.push_back(obj);

        vka::transform T;
        T.set_position( glm::vec3(0,0,0));
        T.set_scale( glm::vec3(1,1,1));


        obj->m_uniform_data.model = vka::transform( glm::vec3( x , y, z)   ).get_matrix(); // T.get_matrix();
        x += 1;
        if( x > 25 )
        {
            x =0;
            y += 1;
        }
        if(y > 25 )
        {
            y = 0;
            z += 1;
        }
    }


   //   m_Objs[1]->m_uniform_data.model = vka::transform( glm::vec3(0,3,0) ).get_matrix(); // T.get_matrix();
   //   m_Objs[2]->m_uniform_data.model = vka::transform( glm::vec3(0,0,4) ).get_matrix(); // T.get_matrix();



    //======================

    //========================
    // Initialize the camera
    glm::vec3 cam_position(4,2,4);
    glm::vec3 cam_looking_at(0,0,0);
    float field_of_view = glm::radians(60.f);
    float aspect_ratio  = WIDTH / (float)HEIGHT;
    float near_plane    = 0.1f;
    float far_plane     = 100.0f;
    m_Camera.set_fov(  field_of_view );
    m_Camera.set_aspect_ratio( aspect_ratio );
    m_Camera.set_near_plane(near_plane); // default value
    m_Camera.set_far_plane(far_plane);// default value
    m_Camera.set_position( cam_position );
    m_Camera.lookat( cam_looking_at , {0,1,0});

    m_frame_uniform.view = m_Camera.get_view_matrix();
    m_frame_uniform.proj = m_Camera.get_proj_matrix();
    m_frame_uniform.proj[1][1] *= -1;

  }

  //=====================================

  vka::descriptor_pool *m_descriptor_pool;
  vka::command_pool    *m_command_pool;

  vka::buffer_pool * m_buffer_pool;

  vka::sub_buffer* m_vbuffer;
  vka::sub_buffer* m_ibuffer;
  vka::sub_buffer* m_ubuffer;
  vka::sub_buffer* m_dbuffer;
  vka::buffer    * m_sbuffer;

  vka::texture2darray * m_texture_array;

  vka::semaphore * m_image_available_semaphore;
  vka::semaphore * m_render_complete_semaphore;

  //========================================
          vka::camera m_Camera;
   per_frame_uniform_t m_frame_uniform;

  struct
  {
    vka::descriptor_set * texture_array;
    vka::descriptor_set * uniform_buffer;
    vka::descriptor_set * dynamic_uniform_buffer;
  } m_dsets;

  struct
  {
    vka::pipeline * main;
    vka::pipeline * line;
  } m_pipelines;


  vka::command_buffer m_command_buffer;

  //====================================

  std::vector< mesh_info_t >        m_mesh_info;
  std::vector< RenderComponent_t* > m_Objs;


};

int main(int argc, char ** argv)
{
     App A;

     A.init( WIDTH, HEIGHT, APP_TITLE);
     A.init_default_renderpass(WIDTH,HEIGHT);

     A.start_mainloop();


     return 0;

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
    #define MAX_OBJECTS 3
    vka::GLFW_Window_Handler m_Window(window);
    std::vector<Object_t>    m_Objects;


    //==========================================================================
    // The camera class is used to help position the camera nd keep track
    // of all the variables associated with it. It also provides methods for
    // moving the camera through space
    //==========================================================================
    vka::camera              m_Camera;
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
    // Create render targets
    //==========================================================================
    {
        auto * Position_Texture = C.new_texture("position_texture");
        Position_Texture->set_format( vk::Format::eR32G32B32A32Sfloat )
                        ->set_size( WIDTH,HEIGHT, 1 )
                        ->set_usage( vk::ImageUsageFlagBits::eSampled| vk::ImageUsageFlagBits::eColorAttachment )
                        ->set_memory_properties( vk::MemoryPropertyFlagBits::eDeviceLocal)
                        ->set_tiling( vk::ImageTiling::eOptimal)
                        ->set_view_type( vk::ImageViewType::e2D )
                        ->create();
        Position_Texture->create_image_view(vk::ImageAspectFlagBits::eColor);

        auto * Normal_Texture = C.new_texture("normal_texture");
        Normal_Texture->set_format( vk::Format::eR32G32B32A32Sfloat )
                ->set_size( WIDTH,HEIGHT,1)
                ->set_usage( vk::ImageUsageFlagBits::eSampled| vk::ImageUsageFlagBits::eColorAttachment )
                ->set_memory_properties( vk::MemoryPropertyFlagBits::eDeviceLocal)
                ->set_tiling( vk::ImageTiling::eOptimal)
                ->set_view_type( vk::ImageViewType::e2D )
                ->create();
        Normal_Texture->create_image_view(vk::ImageAspectFlagBits::eColor);

        auto * Albedo_Texture = C.new_texture("albedo_texture");
        Albedo_Texture->set_format( vk::Format::eR8G8B8A8Unorm )
                ->set_size( WIDTH,HEIGHT,1)
                ->set_usage( vk::ImageUsageFlagBits::eSampled| vk::ImageUsageFlagBits::eColorAttachment )
                ->set_memory_properties( vk::MemoryPropertyFlagBits::eDeviceLocal)
                ->set_tiling( vk::ImageTiling::eOptimal)
                ->set_view_type( vk::ImageViewType::e2D )
                ->create();
        Albedo_Texture->create_image_view(vk::ImageAspectFlagBits::eColor);

        auto * Depth_Texture = C.new_depth_texture("depth_texture_2");
        Depth_Texture->set_size( WIDTH, HEIGHT, 1)
                     ->create();
        Depth_Texture->create_image_view( vk::ImageAspectFlagBits::eDepth);







        vka::renderpass * R2 = C.new_renderpass("renderpass");


        vk::AttachmentDescription a;
        a.samples        = vk::SampleCountFlagBits::e1;      // VK_SAMPLE_COUNT_1_BIT;
        a.loadOp         = vk::AttachmentLoadOp::eClear;     // VK_ATTACHMENT_LOAD_OP_CLEAR;
        a.storeOp        = vk::AttachmentStoreOp::eStore;    // VK_ATTACHMENT_STORE_OP_STORE;
        a.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;  // VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        a.stencilStoreOp = vk::AttachmentStoreOp::eDontCare; // VK_ATTACHMENT_STORE_OP_DONT_CARE;
        a.initialLayout  = vk::ImageLayout::eUndefined;      // VK_IMAGE_LAYOUT_UNDEFINED;
        a.finalLayout    = vk::ImageLayout::eShaderReadOnlyOptimal;  // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


        a.format = Position_Texture->get_format();   R2->add_attachment_description( a );
        a.format = Normal_Texture->get_format();     R2->add_attachment_description( a );
        a.format = Albedo_Texture->get_format();     R2->add_attachment_description( a );

        a.format = Depth_Texture->get_format();
        a.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        R2->add_attachment_description( a );

        R2->add_color_attachment_reference(0, vk::ImageLayout::eColorAttachmentOptimal);
        R2->add_color_attachment_reference(1, vk::ImageLayout::eColorAttachmentOptimal);
        R2->add_color_attachment_reference(2, vk::ImageLayout::eColorAttachmentOptimal);

        R2->add_depth_attachment_reference(3, vk::ImageLayout::eDepthStencilAttachmentOptimal);

        vk::SubpassDependency S0,S1;
        S0.srcSubpass    = VK_SUBPASS_EXTERNAL;
        S0.dstSubpass    = 0;
        S0.srcStageMask  = vk::PipelineStageFlagBits::eBottomOfPipe;
        S0.dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        S0.srcAccessMask = vk::AccessFlagBits::eMemoryRead;
        S0.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
        S0.dependencyFlags = vk::DependencyFlagBits::eByRegion;

        S1.srcSubpass    = 0;
        S1.dstSubpass    = VK_SUBPASS_EXTERNAL;
        S1.srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        S1.dstStageMask  = vk::PipelineStageFlagBits::eBottomOfPipe;
        S1.srcAccessMask = vk::AccessFlagBits::eMemoryRead;
        S1.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
        S1.dependencyFlags = vk::DependencyFlagBits::eByRegion;

        R2->add_subpass_dependency(S0);
        R2->add_subpass_dependency(S1);

        R2->create();

        auto * FB = C.new_framebuffer( "render_target");
        FB->add_attachments(Position_Texture);
        FB->set_extents( vk::Extent2D{WIDTH,HEIGHT} );
        FB->set_renderpass(R2);

        FB->create();
    }
    //==========================================================================



    //==========================================================================
    // Initialize the Command and Descriptor Pools
    //==========================================================================
    vka::descriptor_pool* descriptor_pool = C.new_descriptor_pool("main_desc_pool");
    descriptor_pool->set_pool_size(vk::DescriptorType::eCombinedImageSampler, 2);
    descriptor_pool->set_pool_size(vk::DescriptorType::eUniformBuffer, 1);
    descriptor_pool->set_pool_size(vk::DescriptorType::eUniformBufferDynamic, 1);

    descriptor_pool->create();

    vka::command_pool* cp = C.new_command_pool("main_command_pool");
    //==========================================================================






/*==============================================================================
 Create the Vertex and Index buffers.

 This time we are goign to use a Buffer Pool. Buffer Pools are a VKA specifc
 construct. Simply put, it is esentially a large device-local
 buffer set to be used as vertex|index|uniform.  It is meant to be a large
 buffer big enough to hold many smaller pieces of data.

 Buffer-pools allocate sub_buffers, which hold the same vk::Buffer handle
 as the buffer_pool, but also contains the byte offset into the memory.
 see: https://developer.nvidia.com/vulkan-memory-management

 Sub buffer allocation is handled automatically within the bufferPool. When
 a sub buffer is finished its use, it should free the resource.
==============================================================================*/

        vka::buffer_pool * buf_pool = C.new_buffer_pool("buffer_pool");
        buf_pool->set_size(1024*1024*50); // create 50Mb buffer
        buf_pool->create();
//============================================================================//

        std::vector< mesh_info_t > m_mesh_info;

        std::vector<vka::mesh_t>   meshs;
        meshs.push_back( vka::box_mesh(1,1,1) );
        //meshs.push_back( vka::sphere_mesh(0.5,20,20) );

        // Allocate sub buffers from the buffer pool. These buffers can be
        // used for indices/vertices or uniforms.
        vka::sub_buffer* du_buffer     = buf_pool->new_buffer( 10*1024*1024 );
        vka::sub_buffer* u_buffer      = buf_pool->new_buffer( 10*1024*1024 );
        vka::sub_buffer* vertex_buffer = buf_pool->new_buffer( 10*1024*1024 );
        vka::sub_buffer* index_buffer  = buf_pool->new_buffer( 10*1024*1024 );

        // allocate a staging buffer of 10MB since we will need this to copy
        // data to the sub_buffers
        vka::buffer * staging_buffer = C.new_staging_buffer( "sb", 1024*1024*10 );

        for( auto & M : meshs)
        {

            // sub_buffer::insert inserts data into the sub buffer at an available
            // space and returns a sub_buffer object.

            auto m1v = vertex_buffer->insert( M.vertex_data(), M.vertex_data_size() );
            auto m1i = index_buffer->insert(  M.index_data() , M.index_data_size()  );

            // If you wish to free the memory you have allocated, you shoudl call
            // vertex_buffer->free_buffer_object(m1v);

            assert( m1v.m_size != 0);
            assert( m1i.m_size != 0);

            mesh_info_t m_box_mesh;
            m_box_mesh.count         = M.num_indices();

            // the offset returned is the byte offset, so we need to divide it
            // by the index/vertex size to get the actual index/vertex offset
            m_box_mesh.index_offset  =  m1i.m_offset  / M.index_size();
            m_box_mesh.vertex_offset =  m1v.m_offset  / M.vertex_size();

            m_mesh_info.push_back(m_box_mesh);
        }


        struct line_Vertex
        {
            glm::vec3 p;
            glm::vec4 c;
        };
        std::vector<line_Vertex> X;
        X.resize(6);
        X[0].p = glm::vec3(0);
        X[0].c = glm::vec4(1,0,0,1);
        X[1].p = glm::vec3(1,0,0);
        X[1].c = glm::vec4(1,0,0,1);

        X[2].p = glm::vec3(0);
        X[2].c = glm::vec4(0,1,0,1);
        X[3].p = glm::vec3(0,1,0);
        X[3].c = glm::vec4(0,1,0,1);

        X[4].p = glm::vec3(0);
        X[4].c = glm::vec4(0,0,1,1);
        X[5].p = glm::vec3(0,0,1);
        X[5].c = glm::vec4(0,0,1,1);


        auto L = vertex_buffer->insert( X.data(), X.size()*sizeof(line_Vertex), sizeof(line_Vertex));
        mesh_info_t line_Mesh;
        line_Mesh.count  = 6;
        line_Mesh.index_offset = 0;
        line_Mesh.vertex_offset = L.m_offset / sizeof(line_Vertex);

//==============================================================================
// Create the Texture2dArray
//
//==============================================================================

    // 1. First load host_image into memory, and specifcy we want 4 channels.
        std::vector<std::string> image_paths = {
            "resources/textures/Brick-2852a.jpg",
            "resources/textures/noise.jpg"
        };



    // 2. Use the context's helper function to create a device local texture
    //    We will be using a texture2d which is a case specific version of the
    //    generic texture
        vka::texture2darray * tex = C.new_texture2darray("test_texture");
        tex->set_size( 512 , 512 );
        tex->set_format(vk::Format::eR8G8B8A8Unorm);
        tex->set_mipmap_levels(8);
        tex->set_layers(10);
        tex->create();
        tex->create_image_view(vk::ImageAspectFlagBits::eColor);


        uint32_t layer=0;
        for(auto & img : image_paths)
        {
            vka::host_image D(img, 4);
            assert( D.width() == 512 && D.height()==512);
            tex->copy_image( D , layer++, vk::Offset2D(0,0) );
        }

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

        vka::pipeline* pipeline      = C.new_pipeline("triangle");
        vka::pipeline* line_pipeline = C.new_pipeline("line");

        auto & M = meshs.front();

        // Create the graphics Pipeline
          pipeline->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                  ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

                  ->set_vertex_shader(   vertex_shader )   // the shaders we want to use
                  ->set_fragment_shader( fragment_shader ) // the shaders we want to use

                  // tell the pipeline that attribute 0 contains 3 floats
                  // and the data starts at offset 0
                  ->set_vertex_attribute(0 ,  M.offset( vka::VertexAttribute::ePosition) , M.format( vka::VertexAttribute::ePosition) , M.vertex_size() )
                  // tell the pipeline that attribute 1 contains 3 floats
                  // and the data starts at offset 12
                  ->set_vertex_attribute(1 ,  M.offset( vka::VertexAttribute::eUV)       , M.format( vka::VertexAttribute::eUV) , M.vertex_size() )

                  ->set_vertex_attribute(2 ,  M.offset( vka::VertexAttribute::eNormal)   , M.format( vka::VertexAttribute::eNormal) , M.vertex_size() )

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
                  ->set_render_pass( R )
                  ->create();




            line_pipeline->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                    ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

                    ->set_vertex_shader(   "resources/shaders/line_shader/line_shader_v.spv" , "main") // the shaders we want to use
                    ->set_fragment_shader( "resources/shaders/line_shader/line_shader_f.spv" , "main") // the shaders we want to use

                    // tell the pipeline that attribute 0 contains 3 floats
                    // and the data starts at offset 0
                    ->set_vertex_attribute(0 ,  0 ,  vk::Format::eR32G32B32Sfloat, 7*sizeof(float) )
                    // tell the pipeline that attribute 1 contains 3 floats
                    // and the data starts at offset 12
                    ->set_vertex_attribute(1 ,  3*sizeof(float)       , vk::Format::eR32G32B32A32Sfloat , 7*sizeof(float) )

                    ->set_toplogy( vk::PrimitiveTopology::eLineList)
                    // Triangle vertices are drawn in a counter clockwise manner
                    // using the right hand rule which indicates which face is the
                    // front
                    ->set_front_face(vk::FrontFace::eCounterClockwise)

                    // Cull all back facing triangles.
                    ->set_cull_mode(vk::CullModeFlagBits::eBack)

                    // Tell teh shader that we are going to use a uniform buffer
                    // in Set #0 binding #0
                    ->add_uniform_layout_binding(0, 0, vk::ShaderStageFlagBits::eVertex)

                    // Tell teh shader that we are going to use a uniform buffer
                    // in Set #0 binding #0
                    ->add_dynamic_uniform_layout_binding(1, 0, vk::ShaderStageFlagBits::eVertex)

                    //
                    ->set_render_pass( R )
                    ->create();
//==============================================================================
// Create the objects
//==============================================================================


m_Objects.resize(MAX_OBJECTS);
for(uint32_t j=0;j<MAX_OBJECTS;j++)
{
    double a = (j / (double)MAX_OBJECTS) * 2.0 * 3.14159;
    m_Objects[j].m_mesh          = m_mesh_info[0];
    m_Objects[j].m_pipeline      = pipeline;
    m_Objects[j].m_push.index    = j%2;
    m_Objects[j].m_push.miplevel = -1;
    m_Objects[j].m_transform.set_position(  1.5f*glm::vec3( cos(a), 0, sin(a) ));
}

m_Objects[1].m_mesh = m_mesh_info[0];
m_Objects[0].m_transform.set_position( glm::vec3(3,0,0));
m_Objects[0].m_transform.set_scale( glm::vec3(2,2,2));


m_Objects[1].m_transform.set_position( glm::vec3(0,2,0));
m_Objects[2].m_transform.set_position( glm::vec3(0,0,2));


//==============================================================================

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
    ubuffer_descriptor->attach_uniform_buffer(0, u_buffer, sizeof(uniform_buffer_t), u_buffer->offset());
    ubuffer_descriptor->update();

    vka::descriptor_set * dubuffer_descriptor = pipeline->create_new_descriptor_set(2, descriptor_pool);
    dubuffer_descriptor->attach_dynamic_uniform_buffer(0, du_buffer, sizeof(dynamic_uniform_buffer_t), du_buffer->offset());
    dubuffer_descriptor->update();

    vka::array_view<uniform_buffer_t> staging_buffer_map          = staging_buffer->map<uniform_buffer_t>();
    vka::array_view<dynamic_uniform_buffer_t> staging_dbuffer_map = staging_buffer->map<dynamic_uniform_buffer_t>(sizeof(uniform_buffer_t));




    vka::semaphore * image_available_semaphore = C.new_semaphore("image_available_semaphore");
    vka::semaphore * render_complete_semaphore = C.new_semaphore("render_complete_semaphore");





    // They GLFW_Window_Manager has signals which you can attach functions to.
    // We are going to use a lamda function to determine whenever a key was pressed
    // and then change the Camera's acceleration based on what key was pressed
    //
    // We are going to create a general FPS controller.

    // must save the returned slot otherwise, the signal will be
    // unregistered
    auto keyslot = m_Window.onKey << [&] (vka::Key k, bool down)
    {
        float x=0;
        float y=0;

        if( m_Window.is_pressed(vka::Key::A    ) || m_Window.is_pressed(vka::Key::LEFT )) x += -1;
        if( m_Window.is_pressed(vka::Key::D    ) || m_Window.is_pressed(vka::Key::RIGHT)) x +=  1;
        if( m_Window.is_pressed(vka::Key::W    ) || m_Window.is_pressed(vka::Key::UP   ))   y += -1;
        if( m_Window.is_pressed(vka::Key::S    ) || m_Window.is_pressed(vka::Key::DOWN ))  y +=  1;

        m_Camera.set_acceleration( glm::vec3( x, 0, y ) );

    };

    auto mouseslot =  m_Window.onMouseMove << [&] (double dx, double dy)
    {
      dx = m_Window.mouse_x() - dx;
      dy = m_Window.mouse_y() - dy;
      if( m_Window.is_pressed( vka::Button::RIGHT))
      {
          m_Window.show_cursor(false);
          if( fabs(dx) < 10) m_Camera.yaw(   -dx*0.001f);
          if( fabs(dy) < 10) m_Camera.pitch( dy*0.001f);
      }
      else
      {
          m_Window.show_cursor(true);
      }
      //std::cout << "Moving: " << dx << ", " << dy << std::endl;
    };



    // Initialize the camera
    glm::vec3 cam_position(2,2,2);
    glm::vec3 cam_looking_at(0,0,0);
    float field_of_view = glm::radians(60.f);
    float aspect_ratio  = WIDTH / (float)HEIGHT;
    float near_plane    = 0.1f;
    float far_plane     = 100.0f;
    m_Camera.set_fov(  field_of_view );
    m_Camera.set_aspect_ratio( aspect_ratio );
    m_Camera.set_near_plane(near_plane); // default value
    m_Camera.set_far_plane(far_plane);// default value
    m_Camera.set_position( cam_position );
    m_Camera.lookat( cam_looking_at , {0,1,0});


    // Dynamic Uniform buffers have a set alignment, meaning the number of bytes
    // bound to the pipeline must be a multiple of the alignment
    // In most case the alignment is 256 bytes.
    auto alignment = C.get_physical_device_limits().minUniformBufferOffsetAlignment;

    auto cb = cp->AllocateCommandBuffer();
    while (!glfwWindowShouldClose(window) )
    {
       float t =0.0;// get_elapsed_time();

       // [NEW] We need to call the camera's calculate method periodically
       // this is so that it can perform the proper physics calculations
       // to create smooth motion.
       m_Camera.calculate();

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


      // Copy the uniform buffer data into the staging buffer
      const float AR = WIDTH / ( float )HEIGHT;
      staging_buffer_map[0].view        = m_Camera.get_view_matrix();
      staging_buffer_map[0].proj        = m_Camera.get_proj_matrix();
      staging_buffer_map[0].proj[1][1] *= -1;

      // +------------------------------------------------------+
      // | uniform_data |                                       | Uniform Buffer
      // +------------------------------------------------------+

      // Copy the uniform buffer data from the stating buffer to the uniform sub_buffer. This normally only needs to be done
      // once per rendering frame because it contains frame constant data.
      cb.copySubBuffer( *staging_buffer ,  u_buffer , vk::BufferCopy{ 0,0,sizeof(uniform_buffer_t) } );


      // Copy the dynamic uniform buffer data from the staging buffer
      // to the appropriate offset in the Dynamic Uniform Buffer.
      // +-------------+---------------------------------------+
      // | obj1        | obj2         | obj3...                | Dynamic Uniform Buffer
      // +-------------+---------------------------------------+
      // |<-alignment->|

      for(uint32_t j=0; j < m_Objects.size(); j++)
      {
          // convert the transform into a matrix which will be stored
          // in the uniform struct
          m_Objects[j].m_uniform.model = m_Objects[j].m_transform.get_matrix();


          // copy uniform struct to staging buffer
          staging_dbuffer_map[j]       = m_Objects[j].m_uniform;

          // byte offset within the staging buffer where the data resides
          auto srcOffset = sizeof(uniform_buffer_t) + j * sizeof(dynamic_uniform_buffer_t);
          // byte offset within the dynamic uniform buffer where to copy the data
          auto dstOffset = j * alignment ;//+ du_buffer->m_offset;
          // number of bytes to copy
          auto size      = sizeof(dynamic_uniform_buffer_t);

          cb.copySubBuffer( *staging_buffer , du_buffer , vk::BufferCopy{ srcOffset,dstOffset, size } );

          m_Objects[j].m_uniform_offset = j * alignment ;
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



        cb.bindVertexSubBuffer(0, vertex_buffer);
        cb.bindIndexSubBuffer(index_buffer, vk::IndexType::eUint16);
      //========================================================================
      // Draw all the objects while binding the dynamic uniform buffer
      // to
      //========================================================================
      for(uint32_t j=0 ; j < m_Objects.size(); j++)
      {
          auto & obj = m_Objects[j];

          cb.bindPipeline( vk::PipelineBindPoint::eGraphics, *pipeline );
          // bind the two descriptor sets that we need to that pipeline
           cb.bindDescriptorSet( vk::PipelineBindPoint::eGraphics,
                                                        pipeline,
                                                        0,
                                                        texture_descriptor);

            cb.bindDescriptorSet( vk::PipelineBindPoint::eGraphics,
                                  pipeline,
                                  1,
                                  ubuffer_descriptor );
          cb.bindDescriptorSet(vk::PipelineBindPoint::eGraphics,
                               pipeline,
                               2,
                               dubuffer_descriptor,
                               m_Objects[j].m_uniform_offset);

          cb.pushConstants( pipeline->get_layout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(push_constants_t), &m_Objects[j].m_push);

    // draw 3 indices, 1 time, starting from index 0, using a vertex offset of 0

            cb.drawIndexed(m_Objects[j].m_mesh.count,
                           1,
                           m_Objects[j].m_mesh.index_offset,
                           m_Objects[j].m_mesh.vertex_offset,
                           0);





            cb.bindPipeline( vk::PipelineBindPoint::eGraphics, *line_pipeline );


             cb.bindDescriptorSet( vk::PipelineBindPoint::eGraphics,
                                   line_pipeline,
                                   0,
                                   ubuffer_descriptor );
           cb.bindDescriptorSet(vk::PipelineBindPoint::eGraphics,
                                line_pipeline,
                                1,
                                dubuffer_descriptor,
                                m_Objects[j].m_uniform_offset);
            cb.draw( line_Mesh.count, 1, line_Mesh.vertex_offset,0);

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

