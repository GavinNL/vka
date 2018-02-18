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
#define APP_TITLE "Multibuffer Vertex Attributes"

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
struct per_frame_uniform_t
{
    glm::mat4 view;
    glm::mat4 proj;
#if defined ENABLE_UNIFORM
    int use_uniform = 1;
#else
    int use_uniform = 0;
#endif
};

// This data will be written directly to the command buffer to
// be passed to the shader as a push constant.
struct push_constants_base_t
{
    glm::mat4 model;
    int index; // index into the texture array layer
    int miplevel; // mipmap level we want to use, -1 for shader chosen.
};

struct push_constants_t : public push_constants_base_t
{
    uint8_t _buffer[ 256 - sizeof(push_constants_base_t)];
};

/**
 * @brief The mesh_info_t struct
 * Information about the mesh, number of indices to draw and
 * offset in the index array
 */
struct mesh_info_t
{
    uint32_t index_offset  = 0; // index offset
    uint32_t count         = 0; // number of indices or vertices
    uint32_t vertex_offset = 0; // vertex offset

    std::vector<vka::sub_buffer*> m_buffers;
    vka::sub_buffer              *m_index_buffer=nullptr;

    void bind(vka::command_buffer & cb)
    {
        uint32_t i = 0;
        if( m_index_buffer)
        {
            cb.bindIndexSubBuffer(m_index_buffer, vk::IndexType::eUint16);
        }

        for(auto & b : m_buffers)
            cb.bindVertexSubBuffer( i++, b);
    }

    void draw( vka::command_buffer & cb)
    {
        if( m_index_buffer)
            cb.drawIndexed(count, 1 , index_offset, vertex_offset,0);
        else
            cb.draw(count,1,vertex_offset,0);
    }
};


class PhysicsComponent_t
{
    vka::transform m_transform;
};

class RenderComponent_t
{
public:
    vka::pipeline   *m_pipeline = nullptr;

    std::map<uint32_t, vka::descriptor_set*> m_descriptor_sets;

    push_constants_t         m_push;

    mesh_info_t              m_mesh;

    bool m_draw_axis = true;
};


class FullScreenQuadRenderer_t
{
public:
    vka::pipeline * m_pipeline;
    vka::command_buffer m_commandbuffer;

    FullScreenQuadRenderer_t(vka::command_buffer & cb , vka::pipeline * pipeline) : m_pipeline(pipeline) , m_commandbuffer(cb)
    {
        m_commandbuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, *m_pipeline );
    }

    void operator()( vka::descriptor_set * dset)
    {
        m_commandbuffer.bindDescriptorSet(vk::PipelineBindPoint::eGraphics,
                             m_pipeline,
                             0, // binding index
                             dset);
        m_commandbuffer.draw(6,1,0,0);
        //m_commandbuffer.pushConstants( m_pipeline->get_layout(),
        //                               vk::ShaderStageFlagBits::eVertex,
        //                               0,
        //                               sizeof(glm::mat4),
        //                               &mvp);
    }
};

class AxisRenderer_t
{
public:
    vka::pipeline * m_pipeline;
    vka::command_buffer m_commandbuffer;

    AxisRenderer_t(vka::command_buffer & cb , vka::pipeline * pipeline) : m_pipeline(pipeline) , m_commandbuffer(cb)
    {
        m_commandbuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, *m_pipeline );
    }

    void operator ()( glm::mat4 const & mvp)
    {

        m_commandbuffer.pushConstants( m_pipeline->get_layout(),
                                       vk::ShaderStageFlagBits::eVertex,
                                       0,
                                       sizeof(glm::mat4),
                                       &mvp);
        m_commandbuffer.draw(6,1,0,0);
    }
};

class ComponentRenderer_t
{
    vka::pipeline  *m_pipeline = nullptr;
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
            //m_vbuffer = obj->m_vbuffer;
           // m_ibuffer = obj->m_ibuffer;

            cb.bindPipeline( vk::PipelineBindPoint::eGraphics, *m_pipeline );

            for(auto & d : obj->m_descriptor_sets)
            {
                cb.bindDescriptorSet(vk::PipelineBindPoint::eGraphics,
                                     m_pipeline,
                                     d.first, // binding index
                                     d.second);
            }
        }


        //uint32_t i = 0;
        //for(auto & b : obj->m_mesh.m_buffers)
        //{
        //    cb.bindVertexSubBuffer(i++, b);
        //}
        //cb.bindVertexSubBuffer(1, obj->m_mesh.u_buffer, obj->m_mesh.u_offset);
        //cb.bindVertexSubBuffer(2, obj->m_mesh.n_buffer, obj->m_mesh.n_offset);



        cb.pushConstants( m_pipeline->get_layout(),
                          vk::ShaderStageFlagBits::eVertex,
                          0,
                          sizeof(push_constants_t),
                          &obj->m_push);

        obj->m_mesh.bind(cb);
        obj->m_mesh.draw(cb);
        //mesh_info_t const & m = obj->m_mesh;
        //cb.drawIndexed(m.count, 1 , m.index_offset, m.vertex_offset, 0);
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
      m_descriptor_pool->set_pool_size(vk::DescriptorType::eCombinedImageSampler, 25);
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

      m_pipelines.gbuffer = m_Context.new_pipeline("gbuffer_pipeline");
      m_pipelines.gbuffer->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
              ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

              ->set_vertex_shader(   "resources/shaders/gbuffer/gbuffer_v.spv", "main" )   // the shaders we want to use
              ->set_fragment_shader( "resources/shaders/gbuffer/gbuffer_f.spv", "main" ) // the shaders we want to use

              // tell the pipeline that attribute 0 contains 3 floats
              // and the data starts at offset 0
              ->set_vertex_attribute(0, 0 ,  0 , M.format( vka::VertexAttribute::ePosition) , sizeof(glm::vec3) )
              // tell the pipeline that attribute 1 contains 3 floats
              // and the data starts at offset 12
              ->set_vertex_attribute(1, 1 ,  0       , M.format( vka::VertexAttribute::eUV) , sizeof(glm::vec2) )

              ->set_vertex_attribute(2, 2 ,  0   , M.format( vka::VertexAttribute::eNormal) , sizeof(glm::vec3) )


              ->set_color_attachments( 3 )

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
              ->set_render_pass( m_GBuffer_renderpass );

        m_pipelines.gbuffer->get_color_blend_attachment_state(0).blendEnable=VK_FALSE;
        m_pipelines.gbuffer->get_color_blend_attachment_state(1).blendEnable=VK_FALSE;
        m_pipelines.gbuffer->get_color_blend_attachment_state(2).blendEnable=VK_FALSE;

        m_pipelines.gbuffer->create();

        m_pipelines.main = m_Context.new_pipeline("main_pipeline");
        m_pipelines.main->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

                ->set_vertex_shader(   "resources/shaders/push_consts_default/push_consts_default_v.spv", "main" )   // the shaders we want to use
                ->set_fragment_shader( "resources/shaders/push_consts_default/push_consts_default_f.spv", "main" ) // the shaders we want to use

                // tell the pipeline that attribute 0 contains 3 floats
                // and the data starts at offset 0
                ->set_vertex_attribute(0, 0 ,  0 , M.format( vka::VertexAttribute::ePosition) , sizeof(glm::vec3) )
                // tell the pipeline that attribute 1 contains 3 floats
                // and the data starts at offset 12
                ->set_vertex_attribute(1, 1 ,  0       , M.format( vka::VertexAttribute::eUV) , sizeof(glm::vec2) )

                ->set_vertex_attribute(2, 2 ,  0   , M.format( vka::VertexAttribute::eNormal) , sizeof(glm::vec3) )

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
                ->set_render_pass( m_default_renderpass )
                ->create();


        //======================================================================
        // The axis shader does not require any vertex attributes, it contains
        // 6 vertices, 2 vertices per axis and are drawn as a line list.
        // the vertex positions are generated dynamically in the shader.
        // The shader requires a single push constant which is the
        // cumulative Model-View-Projection matrix.
        //======================================================================
        m_pipelines.axis = m_Context.new_pipeline("axis_pipeline");
        m_pipelines.axis->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

                ->set_vertex_shader(   "resources/shaders/axis_shader/axis_shader_v.spv", "main" )   // the shaders we want to use
                ->set_fragment_shader( "resources/shaders/axis_shader/axis_shader_f.spv", "main" ) // the shaders we want to use

                ->set_toplogy(vk::PrimitiveTopology::eLineList)
                ->set_line_width(5.0f)
                // Triangle vertices are drawn in a counter clockwise manner
                // using the right hand rule which indicates which face is the
                // front
                ->set_front_face(vk::FrontFace::eCounterClockwise)

                // Cull all back facing triangles.
                ->set_cull_mode(vk::CullModeFlagBits::eBack)
                // Add a push constant to the layout. It is accessable in the vertex shader
                // stage only.
                ->add_push_constant( sizeof(glm::mat4), 0, vk::ShaderStageFlagBits::eVertex)
                //
                ->set_render_pass( m_default_renderpass )
                ->create();
        //======================================================================

        //======================================================================
        // The compose pipeline generates a full screen quad (in the shader)
        // and accepts a number of textures. It uses thse textures and combines
        // to produce an image on the screen.
        //======================================================================
        m_pipelines.compose = m_Context.new_pipeline("compose_pipeline");
        m_pipelines.compose->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

                ->set_vertex_shader(   "resources/shaders/compose/compose_v.spv", "main" )   // the shaders we want to use
                ->set_fragment_shader( "resources/shaders/compose/compose_f.spv", "main" ) // the shaders we want to use

                ->set_toplogy(vk::PrimitiveTopology::eTriangleList)
                ->set_line_width(1.0f)
                // Triangle vertices are drawn in a counter clockwise manner
                // using the right hand rule which indicates which face is the
                // front
                ->set_front_face(vk::FrontFace::eCounterClockwise)

                ->add_texture_layout_binding(0, 0, vk::ShaderStageFlagBits::eFragment)
                ->add_texture_layout_binding(0, 1, vk::ShaderStageFlagBits::eFragment)
                ->add_texture_layout_binding(0, 2, vk::ShaderStageFlagBits::eFragment)
                ->add_texture_layout_binding(0, 3, vk::ShaderStageFlagBits::eFragment)

                // Cull all back facing triangles.
                ->set_cull_mode(vk::CullModeFlagBits::eNone)
                // Add a push constant to the layout. It is accessable in the vertex shader
                // stage only.
                ->add_push_constant( sizeof(int), 0, vk::ShaderStageFlagBits::eVertex)
                //
                ->set_render_pass( m_default_renderpass )
                ->create();
        //======================================================================
  }


  void load_meshs()
  {
      std::vector<vka::mesh_t>   meshs;
      meshs.push_back( vka::box_mesh(1,1,1) );
      meshs.push_back( vka::sphere_mesh(0.5,20,20) );

      for( auto & M : meshs)
      {

          auto P = M.get_attribute_view<glm::vec3>( vka::VertexAttribute::ePosition);
          auto U = M.get_attribute_view<glm::vec2>( vka::VertexAttribute::eUV    );
          auto N = M.get_attribute_view<glm::vec3>( vka::VertexAttribute::eNormal);

          std::vector< glm::vec3> p;
          std::vector< glm::vec3> n;
          std::vector< glm::vec2> u;

          for(uint32_t i=0;i<P.size();i++)
          {
                p.push_back( P[i] );
                n.push_back( N[i] );
                u.push_back( U[i] );
          }

         // #error To DO: we cannot just add the data to the the buffer randomly
         //        we need 3 individual subbuffers

          // sub_buffer::insert inserts data into the sub buffer at an available
          // space and returns a sub_buffer object.
          mesh_info_t mesh;

          mesh.m_buffers.push_back( m_buffer_pool->new_buffer( p.size() * sizeof(glm::vec3) , sizeof(glm::vec3 ) ) );
          mesh.m_buffers.push_back( m_buffer_pool->new_buffer( u.size() * sizeof(glm::vec2) , sizeof(glm::vec2 ) ) );
          mesh.m_buffers.push_back( m_buffer_pool->new_buffer( n.size() * sizeof(glm::vec3) , sizeof(glm::vec3 ) ) );
          mesh.m_index_buffer = m_buffer_pool->new_buffer( M.index_data_size(), sizeof(uint16_t) );

          auto m1p = mesh.m_buffers[0]->insert( p.data() , p.size() * sizeof(glm::vec3) , sizeof(glm::vec3 ) );
          auto m1u = mesh.m_buffers[1]->insert( u.data() , u.size() * sizeof(glm::vec2) , sizeof(glm::vec2 ) );
          auto m1n = mesh.m_buffers[2]->insert( n.data() , n.size() * sizeof(glm::vec3) , sizeof(glm::vec3 ) );

          auto m1i = mesh.m_index_buffer->insert(  M.index_data() , M.index_data_size()  , M.index_size() );

          // If you wish to free the memory you have allocated, you shoudl call
          // vertex_buffer->free_buffer_object(m1v);

          //assert( m1v.m_size != 0);
          //assert( m1i.m_size != 0);

          mesh.count         = M.num_indices();

          // the offset returned is the byte offset, so we need to divide it
          // by the index/vertex size to get the actual index/vertex offset
          mesh.index_offset  =  m1i.m_offset  / M.index_size();
          mesh.vertex_offset =  0;

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
      m_dsets.uniform_buffer->attach_uniform_buffer(0, m_ubuffer, sizeof(per_frame_uniform_t), m_ubuffer->offset());
      m_dsets.uniform_buffer->update();


      m_dsets.renderTargets = m_pipelines.compose->create_new_descriptor_set(0, m_descriptor_pool);
      m_dsets.renderTargets->attach_sampler(0, m_textures.renderTargets[0]);
      m_dsets.renderTargets->attach_sampler(1, m_textures.renderTargets[1]);
      m_dsets.renderTargets->attach_sampler(2, m_textures.renderTargets[2]);
      m_dsets.renderTargets->attach_sampler(3, m_textures.renderTargets[3]);
      m_dsets.renderTargets->update();

      // m_dsets.dynamic_uniform_buffer = m_pipelines.main->create_new_descriptor_set(2, m_descriptor_pool);
      // m_dsets.dynamic_uniform_buffer->attach_dynamic_uniform_buffer(0, m_dbuffer, sizeof(dynamic_uniform_buffer_t), m_dbuffer->offset());
      // m_dsets.dynamic_uniform_buffer->update();

  }
  virtual void onInit()
  {
      init_pools();
      init_memory();

      init_render_targets();


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




  void UpdateUniforms(vka::command_buffer & cb)
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



    // Main component renderer
    ComponentRenderer_t R;
    for(auto * comp : m_Objs)
    {
        R(cb, comp);
    }

    // axis renderer
    AxisRenderer_t A(cb, m_pipelines.axis);
    for(auto * comp : m_Objs)
    {

        A( m_frame_uniform.proj * m_frame_uniform.view  * comp->m_push.model );
    }

  //  FullScreenQuadRenderer_t Q(cb, m_pipelines.compose);
  //  Q( m_dsets.renderTargets);

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

    UpdateUniforms(m_command_buffer);
    auto s2 = microseconds();

        // Render
    auto fb_index = UpdateCommandBuffer(m_command_buffer);
    auto s3 = microseconds();
    m_command_buffer.end();

    m_Context.submit_command_buffer(m_command_buffer, m_image_available_semaphore, m_render_complete_semaphore);
    auto s4 = microseconds();

    m_Context.present_image( fb_index , m_render_complete_semaphore);
  //  std::cout <<  s2-s1 << "   " <<  s3-s2 << "   "<<  s4-s3 << "   "<< std::endl;
  }



  void init_scene()
  {
      #define MAX_OBJ 3

      float x = 0;
      float y = 0;
      float z = 0;

      for(int i=0; i<MAX_OBJ ;i++)
      {
        auto * obj = new RenderComponent_t();

        obj->m_mesh     = m_mesh_info[ i%2 ];
        obj->m_pipeline = m_pipelines.main;

        obj->m_descriptor_sets[0] = m_dsets.texture_array;
        obj->m_descriptor_sets[1] = m_dsets.uniform_buffer;

        //obj->m_uniform_buffer = m_dsets.uniform_buffer;
        //obj->m_dynamic_buffer = m_dsets.dynamic_uniform_buffer;
        //obj->m_texture        = m_dsets.texture_array;


        vka::transform T;
        T.set_position( glm::vec3(0,0,0));
        T.set_scale( glm::vec3(1,1,1));


        obj->m_push.model    = vka::transform( 1.25f*glm::vec3( x , y, z)   ).get_matrix(); // T.get_matrix();
        obj->m_push.index    = i%2;
        obj->m_push.miplevel = -1;



        x += 1;
        if( x > pow(MAX_OBJ,1.0/3.0) )
        {
            x =0;
            y += 1;
        }
        if(y > pow(MAX_OBJ,1.0/3.0) )
        {
            y = 0;
            z += 1;
        }

        m_Objs.push_back(obj);
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



  /**
   * @brief build_offscreen_commandbuffer
   * @param cb
   *
   * Renders all the objects to the offscreen framebuffer
   */
  void build_offscreen_commandbuffer( vka::command_buffer & cb )
  {
      //  BEGIN RENDER PASS=====================================================
      // We want the to use the render pass we created earlier
      vk::RenderPassBeginInfo renderPassInfo;
      renderPassInfo.renderPass        = *m_GBuffer_renderpass;
      renderPassInfo.framebuffer       = *m_GBuffer_framebuffer;
      renderPassInfo.renderArea.offset = vk::Offset2D{0,0};
      renderPassInfo.renderArea.extent = vk::Extent2D(WIDTH,HEIGHT);

      // Clear values are used to clear the various frame buffers
      // we want to clear the colour values to black
      // at the start of rendering.
      std::array<vk::ClearValue,4> clearValues;
      clearValues[0].color = vk::ClearColorValue( std::array<float,4>( {0.0f, 0.0f, 0.0f, 0.0f} ) );
      clearValues[1].color = vk::ClearColorValue( std::array<float,4>( {0.0f, 0.0f, 0.0f, 0.0f} ) );
      clearValues[2].color = vk::ClearColorValue( std::array<float,4>( {0.0f, 0.0f, 0.0f, 0.0f} ) );
      clearValues[3].depthStencil = vk::ClearDepthStencilValue(1.0f, 0.0f );

      renderPassInfo.clearValueCount = clearValues.size();
      renderPassInfo.pClearValues    = clearValues.data();

      cb.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
      //========================================================================


      for(auto & c : m_Objs)
      {

      }

      cb.endRenderPass();
  }

  void init_render_targets()
  {
    //==================================================
    /*  Want to be able to do soemthing like this:
        A render target consists of images to render onto, a framebuffer, and
        a renderpass.

      RenderTarget T;
      T.set_size( 1024, 768 );
      T.add_color_attachment( 1024, 768, vk::Format::eR32G32B32A32Sfloat );
      T.add_color_attachment( 1024, 768, vk::Format::eR32G32B32A32Sfloat );
      T.add_color_attachment( 1024, 768, vk::Format::eR8G8B8A8Unorm );
      T.add_depth_attachment( 1024, 768, vk::Format::eR8G8B8A8Unorm );
      T.create();

     */
    //==================================================

          auto * Position_Texture = m_Context.new_texture("position_texture");
          Position_Texture->set_format( vk::Format::eR32G32B32A32Sfloat )
                          ->set_size( WIDTH,HEIGHT, 1 )
                          ->set_usage( vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled| vk::ImageUsageFlagBits::eColorAttachment )
                          ->set_memory_properties( vk::MemoryPropertyFlagBits::eDeviceLocal)
                          ->set_tiling( vk::ImageTiling::eOptimal)
                          ->set_view_type( vk::ImageViewType::e2D )
                          ->create();
          Position_Texture->create_image_view(vk::ImageAspectFlagBits::eColor);
          m_textures.renderTargets[0] = Position_Texture;

          auto * Normal_Texture = m_Context.new_texture("normal_texture");
          Normal_Texture->set_format( vk::Format::eR32G32B32A32Sfloat )
                  ->set_size( WIDTH,HEIGHT,1)
                  ->set_usage(  vk::ImageUsageFlagBits::eTransferDst |vk::ImageUsageFlagBits::eSampled| vk::ImageUsageFlagBits::eColorAttachment )
                  ->set_memory_properties( vk::MemoryPropertyFlagBits::eDeviceLocal)
                  ->set_tiling( vk::ImageTiling::eOptimal)
                  ->set_view_type( vk::ImageViewType::e2D )
                  ->create();
          Normal_Texture->create_image_view(vk::ImageAspectFlagBits::eColor);
          m_textures.renderTargets[1] = Normal_Texture;

          auto * Albedo_Texture = m_Context.new_texture("albedo_texture");
          Albedo_Texture->set_format( vk::Format::eR8G8B8A8Unorm )
                  ->set_size( WIDTH,HEIGHT,1)
                  ->set_usage(  vk::ImageUsageFlagBits::eTransferDst |vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment )
                  ->set_memory_properties( vk::MemoryPropertyFlagBits::eDeviceLocal)
                  ->set_tiling( vk::ImageTiling::eOptimal)
                  ->set_view_type( vk::ImageViewType::e2D )
                  ->create();
          Albedo_Texture->create_image_view(vk::ImageAspectFlagBits::eColor);
m_textures.renderTargets[2] = Albedo_Texture;

          auto * Depth_Texture = m_Context.new_depth_texture("depth_texture_2");
          Depth_Texture->set_size( WIDTH, HEIGHT, 1)
                       ->set_usage(  vk::ImageUsageFlagBits::eTransferDst |vk::ImageUsageFlagBits::eSampled| vk::ImageUsageFlagBits::eDepthStencilAttachment )
                       ->create();
          Depth_Texture->create_image_view( vk::ImageAspectFlagBits::eDepth);
m_textures.renderTargets[3] = Depth_Texture;






          vka::renderpass * R2 = m_Context.new_renderpass("renderpass");


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

          m_GBuffer_renderpass = R2;

          m_GBuffer_framebuffer = m_Context.new_framebuffer( "render_target");
          m_GBuffer_framebuffer->add_attachments(Position_Texture);
          m_GBuffer_framebuffer->add_attachments(Normal_Texture);
          m_GBuffer_framebuffer->add_attachments(Albedo_Texture);
          m_GBuffer_framebuffer->add_attachments(Depth_Texture);
          m_GBuffer_framebuffer->set_extents( vk::Extent2D{WIDTH,HEIGHT} );
          m_GBuffer_framebuffer->set_renderpass(m_GBuffer_renderpass);
          m_GBuffer_framebuffer->create();

  }
  vka::framebuffer * m_GBuffer_framebuffer;
  vka::renderpass  * m_GBuffer_renderpass;
  //=====================================

  vka::descriptor_pool *m_descriptor_pool;
  vka::command_pool    *m_command_pool;

  vka::buffer_pool * m_buffer_pool;

  //vka::sub_buffer* m_vbuffer;
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

    vka::descriptor_set * renderTargets;
  } m_dsets;

  struct textures
  {
    vka::texture * renderTargets[4];
  } m_textures;

  struct
  {
    vka::pipeline * gbuffer;
    vka::pipeline * main;
    vka::pipeline * axis;
    vka::pipeline * compose;
  } m_pipelines;


  vka::command_buffer m_command_buffer;

  //====================================

  std::vector< mesh_info_t >        m_mesh_info;
  std::vector< RenderComponent_t* > m_Objs;

  vka::signal<void(double       , double)>::slot mouseslot;
  vka::signal<void(vka::Key,      int   )>::slot keyslot;

};

int main(int argc, char ** argv)
{
     App A;

     A.init( WIDTH, HEIGHT, APP_TITLE);
     A.init_default_renderpass(WIDTH,HEIGHT);

     A.start_mainloop();


     return 0;
}

#define STB_IMAGE_IMPLEMENTATION
#include<stb/stb_image.h>

