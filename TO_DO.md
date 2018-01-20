Create method for buffers: Copies data to the buffer in a free spot and returns
the offset at which it was copied.

```
// Inserts data into the buffer and returns the offset into the
// buffer at wich the data was inserted (automatically managed)
mem_block_t insert_data( data, size );

// clears the data block at offset, and returns true if the data was cleared
// returns false if that block does not exist anymore
bool        clear_data(size_t offset)
```



