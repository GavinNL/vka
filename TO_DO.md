* Create a method for texture/texture2d/texture2darray copies a host_image
to a texture layer: eg

   tex2d->copy( host_image, vk::Offset2D(0,0) );

   tex2darray->copy( layer_number, host_image, vk::Offset2D(0,0) );

This method will use the context's internal staging buffer to copy the image.
It will convert the texture's layers to the apporpriate layout and then back
into the shaderoptimal
