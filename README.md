# libE57: Software Tools for Managing E57 Files

This is a Git clone of the [Subversion repository][1] for the reference
implementation [libE57][2].

# Changes

It includes changes for building on Linux as distributed in [clausmichele/libE57-linux][3].
They have been applied to the linux branch of this repository and are merged
to main.
Additionally, it also includes the Simple API in the library.

# Instructions

Compile with:

```
mkdir build && cd build
cmake ..
make && sudo make install
```

# License
Please refer to http://libe57.org/license.html


[1][https://svn.code.sf.net/p/e57-3d-imgfmt/code/]
[2][http://www.libe57.org/]
[3][https://github.com/clausmichele/libE57-linux.git]
