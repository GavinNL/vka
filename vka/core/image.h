#ifndef VKA_IMAGE
#define VKA_IMAGE

#include <string.h>
#include <cassert>
#include<type_traits>
#include<functional>
#include<cstdint>
#include<cmath>
#include<stb/stb_image.h>

namespace vka
{
class pixel_color
{
public:
    using raw_type = std::uint8_t;

    pixel_color()
    {
    }

    pixel_color( pixel_color const & other) : value(other.value){}
    explicit pixel_color( raw_type    const & other) : value(other     ){}
    explicit pixel_color(  int16_t    const & other) : value(other&0xFF){}
    explicit pixel_color(  int32_t    const & other) : value(other&0xFF){}
    explicit pixel_color(  int64_t    const & other) : value(other&0xFF){}
    explicit pixel_color( uint16_t    const & other) : value(other&0xFF){}
    explicit pixel_color( uint32_t    const & other) : value(other&0xFF){}
    explicit pixel_color( uint64_t    const & other) : value(other&0xFF){}
    explicit pixel_color( float       const & other) : value(static_cast<raw_type>(other*255) ){}
    explicit pixel_color( double      const & other) : value(static_cast<raw_type>(other*255) ){}

    pixel_color & operator=(pixel_color const & other)
    {
        value = other.value;
        return *this;
    }


    template<class T>
    typename std::enable_if< std::is_floating_point<T>::value, pixel_color >::type&
    operator=(T const & other)
    {
        value = static_cast<raw_type>(other * 255.0);
        return *this;
    }

    template<class T>
    typename std::enable_if< std::is_integral<T>::value, pixel_color >::type&
    operator=(T const & other)
    {
        value = other;
        return *this;
    }



    pixel_color & operator+=(pixel_color const & other)
    {
        value += other.value;
        return *this;
    }
    pixel_color & operator-=(pixel_color const & other)
    {
        value -= other.value;
        return *this;
    }
    pixel_color & operator*=(pixel_color const & other)
    {
        uint32_t v = value * other.value / 255;
        value = static_cast<raw_type>(v);
        return *this;
    }
    pixel_color & operator/=(pixel_color const & other)
    {
        uint32_t v =  (static_cast<float>(value) / static_cast<float>(value)) * 255;
        value = static_cast<raw_type>(v);
        return *this;
    }

    template<class T>
    typename std::enable_if< std::is_integral<T>::value, pixel_color >::type&
    operator-=(T const & other)
    {
        value -= pixel_color(other).value;
        return *this;
    }
    template<class T>
    typename std::enable_if< std::is_integral<T>::value, pixel_color >::type&
    operator+=(T const & other)
    {
        value += pixel_color(other).value;
        return *this;
    }


    template<class T>
    typename std::enable_if< std::is_floating_point<T>::value, pixel_color >::type&
    operator-=(T const & other)
    {
        value -= pixel_color(other).value;
        return *this;
    }
    template<class T>
    typename std::enable_if< std::is_floating_point<T>::value, pixel_color >::type&
    operator+=(T const & other)
    {
        value += pixel_color(other).value;
        return *this;
    }


    template<class T>
    typename std::enable_if< std::is_floating_point<T>::value, pixel_color >::type&
    operator*=(T const & other)
    {
        value *= other;
        return *this;
    }
    template<class T>
    typename std::enable_if< std::is_integral<T>::value, pixel_color >::type&
    operator*=(T const & other)
    {
        value /= other;
        return *this;
    }



    template<class T>
    typename std::enable_if< std::is_floating_point<T>::value, pixel_color >::type&
    operator/=(T const & other)
    {
        value /= other;
        return *this;
    }

    template<class T>
    typename std::enable_if< std::is_integral<T>::value, pixel_color >::type&
    operator/=(T const & other)
    {
        value /= other;
        return *this;
    }

    operator float()  const  { return value / 255.0f; }
    operator raw_type() const  { return value; }

    //======================

    raw_type value;
};


inline pixel_color operator+(pixel_color const & l, pixel_color const & r)
{
    return pixel_color( static_cast<pixel_color::raw_type>(l.value+r.value) );
}
inline pixel_color operator-(pixel_color const & l, pixel_color const & r)
{
    return pixel_color( static_cast<pixel_color::raw_type>(l.value - r.value) );
}
inline pixel_color operator*(pixel_color const & l, pixel_color const & r)
{
    return pixel_color( static_cast<pixel_color::raw_type>( l.value*r.value/255.0f) );
}
inline pixel_color operator/(pixel_color const & l, pixel_color const & r)
{
    return pixel_color( static_cast<pixel_color::raw_type>( l.value*255.0f/r.value) );
}

template<class T>
inline typename std::enable_if< std::is_floating_point<T>::value, pixel_color >::type
operator * ( pixel_color const & l, T const & r)
{
    return pixel_color( static_cast<pixel_color::raw_type>(l.value*r) );
}
template<class T>
inline typename std::enable_if< std::is_floating_point<T>::value, pixel_color >::type
operator * ( T const & l, pixel_color const & r)
{
    return pixel_color( static_cast<pixel_color::raw_type>(l * r.value) );
}
template<class T>
inline typename std::enable_if< std::is_integral<T>::value, pixel_color >::type
operator * ( pixel_color const & l, T const & r)
{
    return pixel_color( static_cast<pixel_color::raw_type>(l.value*r) );
}
template<class T>
inline typename std::enable_if< std::is_integral<T>::value, pixel_color >::type
operator * ( T const & l, pixel_color const & r)
{
    return pixel_color( static_cast<pixel_color::raw_type>(l * r.value) );
}




template<class T>
inline typename std::enable_if< std::is_floating_point<T>::value, pixel_color >::type
operator / ( pixel_color const & l, T const & r)
{
    return pixel_color( static_cast<pixel_color::raw_type>(l.value/r) );
}
template<class T>
inline typename std::enable_if< std::is_floating_point<T>::value, pixel_color >::type
operator / ( T const & l, pixel_color const & r)
{
    return pixel_color( static_cast<pixel_color::raw_type>(l / r.value) );
}
template<class T>
inline typename std::enable_if< std::is_integral<T>::value, pixel_color >::type
operator / ( pixel_color const & l, T const & r)
{
    return pixel_color( static_cast<pixel_color::raw_type>(l.value/r) );
}
template<class T>
inline typename std::enable_if< std::is_integral<T>::value, pixel_color >::type
operator / ( T const & l, pixel_color const & r)
{
    return pixel_color( static_cast<pixel_color::raw_type>(l / r.value) );
}



#define CPM(op) \
inline bool operator op (pixel_color const & l, pixel_color const & r )\
{                                                                    \
    return l.value op r.value;                                         \
}                                                                    \
template<typename T>                                                 \
inline bool operator op (pixel_color const & l, T const & r )          \
{                                                                    \
    return l op pixel_color(r);                                      \
}                                                                    \
template<typename T>                                                 \
inline bool operator op (T const & l, pixel_color const & r )          \
{                                                                    \
    return pixel_color(l) op r;                                      \
}

CPM(==)
CPM(!=)
CPM(<)
CPM(>)
CPM(>=)
CPM(<=)

class image;

class channel
{
public:
    using element_type = pixel_color;

    channel(uint32_t & w, uint32_t & h, uint32_t & c , element_type *& data, uint32_t ch)
        :
          m_width(w), m_height(h), m_channels(c), m_data(data), m_channel(ch)
    {

    }

    channel & operator=(channel const & other)
    {
        if( this != &other)
        {
            assert( m_width == other.m_width && m_height == other.m_height);
            for(uint32_t r = 0; r < m_height; ++r)
            {
                for(uint32_t c = 0; c < m_height; ++c)
                {
                    (*this)(r,c) = other(r,c);
                }
            }
        }
        return *this;
    }

    element_type const & operator()(uint32_t r, uint32_t c) const
    {
        return m_data[ (r*m_width + c)*m_channels + m_channel];
    }
    element_type & operator()(uint32_t r, uint32_t c)
    {
        return m_data[ (r*m_width + c)*m_channels + m_channel];
    }

    element_type & operator[](uint32_t index)
    {
        return m_data[index*m_channels];
    }
    element_type const & operator[](uint32_t index) const
    {
        return m_data[index*m_channels];
    }

    //============ ASSIGNMENT OPERATORS ==============================
    template<class T>
    typename std::enable_if< std::is_floating_point<T>::value || std::is_integral<T>::value, channel >::type&
    operator = ( T const & other)
    {
        for(uint32_t r = 0; r < m_height; ++r)
            for(uint32_t c = 0; c < m_width; ++c)
                    (*this)(r,c) = other;
        return *this;
    }

    //============= inc/decrement operators ===========================
#define INC_OP(op) \
    template<class T> \
    typename std::enable_if< std::is_floating_point<T>::value || std::is_integral<T>::value, channel >::type& \
    operator op ( T const & other) \
    { \
        for(uint32_t r = 0; r < m_height; ++r) \
            for(uint32_t c = 0; c < m_width; ++c) \
                    (*this)(r,c) op other; \
        return *this; \
    } \
    template<class T> \
    typename std::enable_if< std::is_floating_point<T>::value || std::is_integral<T>::value, channel >::type& \
    operator op (channel const & other) \
    { \
        assert( m_width == other.m_width && m_height == other.m_height); \
        for(uint32_t r = 0; r < m_height; ++r) \
            for(uint32_t c = 0; c < m_width; ++c) \
            { \
                (*this)(r,c) op other(r,c); \
            } \
        return *this; \
    }

    channel & operator = ( image const & other);

INC_OP(+=)
INC_OP(-=)
INC_OP(*=)
INC_OP(/=)
#undef INC_OP



    channel & operator = ( std::function< float(float, float)> f)
    {
        auto w = width();
        auto h = height();

        float wf = 1.0f / w;
        float hf = 1.0f / h;

        float u = 0.0f;
        float v = 0.0f;

        for(uint32_t i=0 ; i<h; ++i)
        {
            for(uint32_t j=0 ; j<w; ++j)
            {
                auto p        =  f(u,v);
                u            +=  wf;
                (*this)(i,j)  =  p;
            }
            u  = 0.0f;
            v += hf;
        }
        return *this;
    }

    #define uv_exp(exper) [] (float u, float v) { return exper; }

    std::uint32_t width() const { return m_width; }
    std::uint32_t height() const { return m_height; }

    std::uint32_t  & m_width;
    std::uint32_t  & m_height;
    std::uint32_t  & m_channels;
    element_type * & m_data;
    std::uint32_t    m_channel;

};


class image
{
    using element_type = pixel_color;

public:
    image() :
        r(m_width,m_height,m_channels, m_data, 0),
        g(m_width,m_height,m_channels, m_data, 1),
        b(m_width,m_height,m_channels, m_data, 2),
        a(m_width,m_height,m_channels, m_data, 3)
    {
    }

    image(uint32_t w, uint32_t h, uint32_t channels=4) :
        r(m_width,m_height,m_channels, m_data, 0<channels?0:0 ),
        g(m_width,m_height,m_channels, m_data, 1<channels?1:0 ),
        b(m_width,m_height,m_channels, m_data, 2<channels?2:0 ),
        a(m_width,m_height,m_channels, m_data, 3<channels?3:0 )
    {
        allocate(w,h,channels,true);
    }

    image(image const & other) :
        r(m_width,m_height,m_channels, m_data, 0<m_channels?0:0),
        g(m_width,m_height,m_channels, m_data, 1<m_channels?1:0),
        b(m_width,m_height,m_channels, m_data, 2<m_channels?2:0),
        a(m_width,m_height,m_channels, m_data, 3<m_channels?3:0)
    {
        allocate( other.m_width, other.m_height, other.m_channels);

        memcpy( data(), other.data(), size() );

        update_channel_references();
    }
    image(image && other) :
        r(m_width,m_height,m_channels, m_data, 0<m_channels?0:0),
        g(m_width,m_height,m_channels, m_data, 1<m_channels?1:0),
        b(m_width,m_height,m_channels, m_data, 2<m_channels?2:0),
        a(m_width,m_height,m_channels, m_data, 3<m_channels?3:0)
    {
        m_data       = other.m_data;
        m_width      = other.m_width;
        m_height     = other.m_height;
        m_channels   = other.m_channels;

        other.m_data      = nullptr;
        other.m_width     = 0;
        other.m_height    = 0;
        other.m_channels  = 0;

        update_channel_references();
    }


    image( void const * data, uint32_t width, uint32_t height, uint32_t channels):
        r(m_width,m_height,m_channels, m_data, 0<m_channels?0:0),
        g(m_width,m_height,m_channels, m_data, 1<m_channels?1:0),
        b(m_width,m_height,m_channels, m_data, 2<m_channels?2:0),
        a(m_width,m_height,m_channels, m_data, 3<m_channels?3:0)
    {
        allocate( width, height, channels);
        update_channel_references();
        memcpy( m_data, data, width*height*channels);
    }

    ~image()
    {
        free_memory();
    }

    image(const std::string & path, uint32_t channels) :
        r(m_width,m_height,m_channels, m_data, 0<m_channels?0:0),
        g(m_width,m_height,m_channels, m_data, 1<m_channels?1:0),
        b(m_width,m_height,m_channels, m_data, 2<m_channels?2:0),
        a(m_width,m_height,m_channels, m_data, 3<m_channels?3:0)
    {
        load_from_path(path, channels);
        update_channel_references();
    }

    void load_from_path(const std::string & path, uint32_t ForceNumberChannels)
    {
        int x, y, comp;

        //==========================================================
        // Load the ImageBase from an image file.
        //==========================================================
        //unsigned char * img = ImageLoader::GLA_load(path.c_str(), &x, &y, &comp, ForceNumberChannels );
        auto * img = stbi_load(path.c_str(), &x, &y, &comp, ForceNumberChannels );
        if( img )
        {
            allocate( x , y , ForceNumberChannels , false);
            memcpy( m_data, img, x*y*ForceNumberChannels );
            stbi_image_free(img);
            update_channel_references();

        } else {
            //GLA_DOUT  << "Error loading Image: " << path << std::endl;
            throw std::runtime_error( std::string("Error loading Image: ") + path);
        }

    }

    image & operator=(image const & other)
    {
        if( this != &other)
        {
            m_width    = other.m_width;
            m_height   = other.m_height;
            m_channels = other.m_channels;

            allocate( m_width, m_height, m_channels);

            memcpy( data(), other.data(), size() );

            update_channel_references();

        }
        return *this;
    }
    image & operator=(image && other)
    {
        if( this != &other)
        {
            m_data = other.m_data;
            m_width = other.m_width;
            m_height = other.m_height;
            m_channels = other.m_channels;
            other.m_data = nullptr;
            other.m_width = 0;
            other.m_height = 0;
            other.m_channels=0;

            update_channel_references();
        }
        return *this;
    }

    template<class T>
    typename std::enable_if< std::is_floating_point<T>::value || std::is_integral<T>::value, image >::type&
    operator=( T const & other)
    {
        auto s = size();
        for(std::size_t i=0;i<s;i++)
        {
            m_data[i] = other;
        }
        return *this;
    }

    image & operator=( channel const & other)
    {
        assert( width()  == other.width() );
        assert( height() == other.height() );

        auto s = size();
        auto c = channels();

        for(std::size_t i=0;i<c;i++)
            (*this)[i] = other;

        return *this;
    }
    void update_channel_references()
    {
        r.m_channel = m_channels>=1 ? 0:0;
        g.m_channel = m_channels>=2 ? 1:0;
        b.m_channel = m_channels>=3 ? 2:0;
        a.m_channel = m_channels>=4 ? 3:0;
    }

    void free_memory()
    {
        if( m_data )
        {
            delete [] m_data;
            m_width = m_height = m_channels = 0;
            m_data = nullptr;
        }
    }
    bool allocate(uint32_t w, uint32_t h, uint32_t channels, bool erase=false)
    {
        if( w*h*channels == 0)
            return false;

        free_memory();

        m_data     = new element_type[ w * h * channels ];
        m_width    = w;
        m_height   = h;
        m_channels = channels;
        if(erase)
            memset(m_data, 0 , size() );
        return true;
    }

    element_type & operator()(uint32_t row, uint32_t col)
    {
        return m_data[ (row*m_width+col)*m_channels ];
    }
    element_type & operator()(uint32_t row, uint32_t col, uint32_t ch)
    {
        return m_data[ (row*m_width+col)*m_channels+ch ];
    }
    element_type const & operator()(uint32_t row, uint32_t col, uint32_t ch) const
    {
        return m_data[ (row*m_width+col)*m_channels+ch ];
    }
    channel & operator[](uint32_t ch)
    {
        return (&r)[ch];
    }
    channel const & operator[](uint32_t ch) const
    {
        return (&r)[ch];
    }

    void print()
    {
        for(uint32_t i = 0; i < height(); i++)
        {
            for(uint32_t j = 0; j < width(); j++)
            {
                for(uint32_t k = 0; k < channels(); k++)
                {
                    printf("%02x", (*this)(i,j,k).value );
                }
                printf(" ");
            }
            printf("\n");
        }
         printf("\n");

    }


    inline void resize( uint32_t width, uint32_t height)
    {
        image T( width, height, channels() );

        auto w = T.width();
        auto h = T.height();

        for(auto j=0u; j < h ; j++)
        {
            for(auto i=0u; i < w; i++)
            {
                // scale the x and y dimeninos to be between 0 and 1
                float x = (float)i / (float)w;
                float y = (float)j / (float)h;

                // Find the index on the main Image
                int X = static_cast<int>(x * (float)m_width);
                int Y = static_cast<int>(y * (float)m_height);

                float s = x * static_cast<float>(m_width);
                float t = y * static_cast<float>(m_height);

                // get the fractional part
                s = s-floor(s);
                t = t-floor(t);

                for(auto z=0u; z < m_channels; z++)
                {

                    //assert( X<=1024 );
                    // sample the 4 locations
                    float f00 = (*this)(Y  ,X    , z );
                    float f01 = (*this)(Y  ,X+1  , z );
                    float f10 = (*this)(Y+1,X    , z );
                    float f11 = (*this)(Y+1,X+1  , z );


                    //T(i,j,z) = static_cast<unsigned char>(f00 * (1-s)*(1-t) + f10*s*(1-t) + f01*(1-s)*t + f11*s*t);
                    T(i,j,z) = f00 * (1-s)*(1-t) + f10*s*(1-t) + f01*(1-s)*t + f11*s*t;

                }
            }
        }

        //GLA_DOUT  << "Resizing complete" << std::endl;

        *this = std::move(T);
    }


    element_type const * data() const { return m_data; }
    element_type * data() { return m_data; }
    size_t         size() const { return m_width*m_height*m_channels;}

    uint32_t width()  const { return m_width;}
    uint32_t height() const { return m_height;}
    uint32_t channels() const { return m_channels;}

    uint32_t m_width    = 0;
    uint32_t m_height   = 0;
    uint32_t m_channels = 0;

    channel r;
    channel g;
    channel b;
    channel a;

    element_type * m_data = nullptr;
};

#define OP(op) \
inline image operator op ( const channel & left, const channel & right)\
{                                                                     \
    image Img( left.width(), left.width(),1);                         \
    for(uint32_t i=0; i < Img.width(); ++i)                           \
    {                                                                 \
        for(uint32_t j=0; j < Img.height(); ++j)                      \
        {                                                             \
            Img.r(i,j) = left(i,j) op right(i,j);                      \
        }                                                             \
    }                                                                 \
    return Img;                                                       \
} \
template<typename T> \
inline image operator op ( const T & left, const channel & right)\
{                                                                     \
    image Img( left.width(), left.width(),1);                         \
    for(uint32_t i=0; i < Img.width(); ++i)                           \
    {                                                                 \
        for(uint32_t j=0; j < Img.height(); ++j)                      \
        {                                                             \
            Img.r(i,j) = left op right(i,j);                      \
        }                                                             \
    }                                                                 \
    return Img;                                                       \
}\
template<typename T> \
inline image operator op ( const channel & left, const T & right)\
{                                                                     \
    image Img( left.width(), left.width(),1);                         \
    for(uint32_t i=0; i < Img.width(); ++i)                           \
    {                                                                 \
        for(uint32_t j=0; j < Img.height(); ++j)                      \
        {                                                             \
            Img.r(i,j) = left(i,j) op right;                          \
        }                                                             \
    }                                                                 \
    return Img;                                                       \
} \
inline image operator op ( const image & left, const image & right)\
{                                                                     \
    assert( left.channels() == right.channels() ); \
    assert( left.width() == right.width() ); \
    assert( left.height() == right.height() ); \
    image Img( left.width(), left.width(), left.channels() );                         \
    for(uint32_t i=0;i<Img.size();++i) \
        Img.data()[i] = left.data()[i] op right.data()[i];  \
    return Img;                                                       \
}

OP(+)
OP(-)
OP(*)
OP(/)
#undef OP

inline channel & channel::operator = ( image const & other)
{
    uint32_t c = other.channels();
    for(uint32_t i=0; i < m_height; ++i)
    {
        for(uint32_t j=0; j < m_width; ++j)
        {
            uint32_t avg = 0;
            for(uint32_t k=0; k < c; ++k)
                avg += other(i,j,k).value;

            avg /= c;
            (*this)(i,j) = avg;
        }
    }
    return *this;
}

using host_image = vka::image;
}

#endif
