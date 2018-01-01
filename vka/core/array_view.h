#ifndef VKA_ARRAY_VIEW_H
#define VKA_ARRAY_VIEW_H

#include <cstdint>
#include <cassert>


namespace vka
{


template <typename T>
class array_view
{
public:
  array_view(std::nullptr_t)
    : m_count(0)
    , m_ptr(nullptr)
    , m_alignment(sizeof(T))
  {}

  array_view(T & ptr)
    : m_count(1)
    , m_ptr(&ptr)
    , m_alignment(sizeof(T))
  {}

  array_view(uint32_t count, void * ptr)
    : m_count(count)
    , m_ptr(ptr)
    , m_alignment(sizeof(T))
  {}

  array_view(uint32_t count, void * ptr, size_t alignment)
    : m_count(count)
    , m_ptr(ptr)
    , m_alignment( sizeof(T) > alignment ? sizeof(T) : alignment )
  {}

  template <size_t N>
  array_view(std::array<typename std::remove_const<T>::type, N> & data)
    : m_count(N)
    , m_ptr(data.data())
    , m_alignment(sizeof(T))
  {}

  template <size_t N>
  array_view(std::array<typename std::remove_const<T>::type, N> const& data)
    : m_count(N)
    , m_ptr(data.data())
  {}

  template <class Allocator = std::allocator<typename std::remove_const<T>::type>>
  array_view(std::vector<typename std::remove_const<T>::type, Allocator> & data)
    : m_count(static_cast<uint32_t>(data.size()))
    , m_ptr(data.data())
  {}

  template <class Allocator = std::allocator<typename std::remove_const<T>::type>>
  array_view(std::vector<typename std::remove_const<T>::type, Allocator> const& data)
    : m_count(static_cast<uint32_t>(data.size()))
    , m_ptr(data.data())
  {}

  array_view(std::initializer_list<T> const& data)
    : m_count(static_cast<uint32_t>(data.end() - data.begin()))
    , m_ptr(data.begin())
  {}

  const T * begin() const
  {
    return m_ptr;
  }

  const T * end() const
  {
    return m_ptr + m_count;
  }

  const T & front() const
  {
    assert(m_count && m_ptr);
    return *m_ptr;
  }

  const T & back() const
  {
    assert(m_count && m_ptr);
    return *(m_ptr + m_count - 1);
  }

  bool empty() const
  {
    return (m_count == 0);
  }

  uint32_t size() const
  {
    return m_count;
  }

  T * data() const
  {
    return m_ptr;
  }

  T & operator[] (int32_t i)
  {
      unsigned char * const c = reinterpret_cast<unsigned char*>(m_ptr);
      return *reinterpret_cast<T*>( c + i*m_alignment);
  }

private:
  uint32_t  m_count;
  void *    m_ptr;
  uint32_t  m_alignment;
};


}


#endif
