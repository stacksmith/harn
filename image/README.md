# Image directory

`sources.txt` file, containing the entire history of every compilation, will be stored here.

When serialized with the `save` command, this directory will contain `image.dat`, the image file containing the artifacts and the metadata.

`load` will restore the system state; do so after restart for now.  Make sure the sources file is matching the saved image.

The two files are connected; the artifacts refer to specific sources file offsets; make sure to keep both together.  
