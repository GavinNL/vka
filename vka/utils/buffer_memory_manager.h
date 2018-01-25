#ifndef VKA_BUFFER_MEMORY_MANAGER_H
#define VKA_BUFFER_MEMORY_MANAGER_H

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <iostream>
#include <cassert>
#include <exception>
#include <list>

namespace vka
{

/**
 * @brief The buffer_memory_manager class
 *
 * A buffer memory manager is a class which keeps
 * track of where allocated and free memory are
 * in a block of contigiuos memory.
 *
 */
class buffer_memory_manager
{

    struct info
    {
      size_t m_size      = 0;
      size_t m_offset    = 0;
      bool   m_allocated = false;
      info * m_next = nullptr;
      info * m_prev = nullptr;

      std::string to_string(char c, size_t block_size=1)
      {
          if( empty() )
              return std::string(m_size/block_size, '.');
          else
          {
              return std::string(m_size/block_size, c);
          }
      }

      bool empty()
      {
        return !m_allocated;
      }



    };
public:

    static constexpr size_t error = std::numeric_limits<size_t>::max();

    buffer_memory_manager()
    {
        m_list.clear();
    }

    buffer_memory_manager(size_t i)
    {
        reset(i);
    }

    operator bool() const
    {
        return m_list.size()!=0;
    }

    void reset(size_t total_bytes)
    {
        m_list.clear();
        info k;
        k.m_size      = total_bytes;
        k.m_offset    = 0;
        k.m_allocated = false;

        m_list.push_front(k);
    }

    size_t num_blocks() const
    {
        return m_list.size();
    }

    /**
     * @brief free
     * @param offset
     *
     * Frees the the block which is at the given offset.
     * If no block exists, it will throw an error
     */
    void free(size_t offset)
    {
        auto i = m_list.begin();
        while( i != m_list.end() )
        {
            if( i->m_offset == offset )
            {
                i->m_allocated = false;


                //======== loop back until we are the first free block;

                while( i != m_list.begin() )
                {
                    i--;
                    if( !i->empty() )
                    {
                        i++;
                        forward_merge(i);
                        return;
                    }
                }

                forward_merge(i);


                return;
            }
            ++i;
        }
        throw std::runtime_error("Bad free");
    }

    void print(size_t block_size=1)
    {
        static size_t i=0;
        char s[] = {'#','&','@'};

        size_t lc=0;

        size_t L = 64;

        std::string str;
        str.reserve(10000);

        for(auto & f : m_list)
        {
           str += f.to_string( s[i%sizeof(s)], block_size) ;
           i++;
        }

        i=0;
        while(i < str.size() )
        {
            std::cout << str.substr(i,L) << std::endl;
            i += L;
        }
        std::cout << std::endl;
    }


    size_t allocate_at(size_t n, size_t offset)
    {
        return allocate_at( m_list.begin(), n, offset);
    }

    size_t allocate_at(std::list<info>::iterator i, size_t n, size_t offset)
    {
      //  auto i = m_list.begin();
        while( i != m_list.end() )
        {
            if(i->empty() )
            {
                if( offset >= i->m_offset && offset+n <= i->m_offset+i->m_size)
                {
                    info left_empty;
                    left_empty.m_size = offset - i->m_offset;
                    left_empty.m_offset = offset;
                    left_empty.m_allocated = false;

                    info middle_full;
                    middle_full.m_size   = n;
                    middle_full.m_offset = offset;
                    middle_full.m_allocated = true;

                    info right_empty;
                    right_empty.m_size   = i->m_size - left_empty.m_size - n;
                    right_empty.m_offset = middle_full.m_offset + middle_full.m_size;
                    right_empty.m_allocated = false;


                    if(left_empty.m_size!=0) m_list.insert(i, left_empty);
                    m_list.insert(i, middle_full);
                    *i = right_empty;

                    if(i->m_size==0)
                        m_list.erase(i);
                    return offset;
                }
            }
            i++;
        }
        return error;
    }
    /**
     * @brief allocate
     * @param n
     * @param offset
     * @return
     *
     * allocate a block of memeory and return the offset
     * at which it allocated.
     *
     * If providing
     */
    size_t allocate(size_t n, size_t alignment=1)
    {

        auto i = m_list.begin();
        while( i != m_list.end() )
        {
            if( i->empty()  )
            {
                size_t allocation_offset = (i->m_offset/alignment + (i->m_offset%alignment == 0 ? 0 : 1))*alignment;


                //if( n <= i->m_size  + (i->m_offset%alignment) ) // the whole block can fit in this
                if( allocation_offset + n <= i->m_offset + i->m_size )
                {
                   // auto aligned_offset = i->m_offset + (i->m_offset%alignment); // current location to allocate

                    auto r = allocate_at(i, n , allocation_offset);
                    return r;
                }
            }
            ++i;
        }
        return error;
    }


private:
    std::list<info> m_list;

    void forward_merge(  std::list<info>::iterator iterator)
    {
        auto next = iterator;
        next++;

        if( next != m_list.end())
        {
            if( iterator->empty() && next->empty() )
            {
                iterator->m_size += next->m_size;
                m_list.erase(next);
                forward_merge(iterator);
                //iterator--;
                //forward_merge( iterator );
            }
        }
    }
};


}


#endif
