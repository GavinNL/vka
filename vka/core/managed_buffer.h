#ifndef VKA_MANAGED_BUFFER_H
#define VKA_MANAGED_BUFFER_H


#include "buffer.h"
#include <vka/utils/buffer_memory_manager.h>

namespace vka
{

class context;

/**
 * @brief The managed_buffer class
 *
 * The managed buffer class is child class of buffer.
 *
 * managed_buffer provides a new copy( ) function to copy
 * data to teh buffer. It's copy function finds an appropriate
 * spot in the buffer to copy the data to and then gives you
 * the offset into the buffer where it copied to.
 *
 * Calling managed_buffer.free(offset) releases the allocated
 * block at offset and allows it to be allocated by subsequent
 * calls.
 *
 */
class managed_buffer : public buffer
{
protected:
    managed_buffer( context* parent) : buffer(parent)
    {}

public:


    /**
     * @brief copy
     * @param data - pointer to the data to copy
     * @param _size - size in bytes of the data
     * @param alignment - alignment of the data
     * @return offset into the buffer, or -1 if error
     *
     * Copies data to the buffer and returns the offset into the
     * buffer where it was copied to. You should not copy data
     * to this buffer using any other method.
     */
    size_t copy( void const * data, size_t _size, size_t alignment=1)
    {
        if( !m_manager )
        {
            m_manager.reset( this->size() );
        }

        auto offset = m_manager.allocate(_size,alignment);
        if( offset != m_manager.error)
        {
            this->buffer::copy(data,_size,offset);
            return offset;
        }
        return m_manager.error;
    }


    /**
     * @brief free
     * @param offset
     * @return
     *
     * Free the memory block at offset. This offset value must
     * have been returned by a prevous call to copy( )
     *
     * Attempting to free an offset which was not allocated will
     * throw an error
     */
    void free( size_t offset )
    {
        m_manager.free(offset);
    }


protected:

    buffer_memory_manager m_manager;

    friend class context;
    friend class deleter<buffer>;

private:
    using buffer::copy;


};


}

#endif
