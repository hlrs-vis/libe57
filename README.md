# libE57-linux
Slightly modified version of libE57 to make it work on Linux. Tested on Ubuntu 18.04.

# Instructions

Download libE57 source code from here: http://sourceforge.net/projects/e57-3d-imgfmt/files/E57Refimpl-src/E57RefImpl_src-1.1.312.zip/download

Unpack the zip and replace:
- CMakeLists.txt in the project's root directory
- e57fields.cpp in ./src/tools/
- E57FoundationImpl.cpp in ./src/refimpl/

Compile with:

```
mkdir build && cd build
cmake ..
sudo make && sudo make install
```

# License
Please refer to http://libe57.org/license.html

# Credits

https://gist.github.com/andersgb/c62bb013b22ee57ee0c92b0752a22df6/revisions

https://github.com/POV-Ray/povray/issues/8
