# libE57: Software Tools for Managing E57 Files

This is a Git clone of the [Subversion repository][1] for the reference
implementation [libE57][2].

# Changes

- also the Simple API is included in the library
- changes for building on Linux as distributed in [clausmichele/libE57-linux][3],
  applied to the linux branch of this repository and merged into main
- build fixes for macOS from [arnaudbletterer/E57][4]
- version bump

# Instructions

Compile with:

```
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make && sudo make install
```

# License
Please refer to the [license.html][5] on the libe57 website.


[1]: https://svn.code.sf.net/p/e57-3d-imgfmt/code/
[2]: http://www.libe57.org/
[3]: https://github.com/clausmichele/libE57-linux.git
[4]: https://github.com/arnaudbletterer/E57.git
[5]: http://libe57.org/license.html
