Project make files for VC 8.0.

To use the file, open the e57-3d-imgfmt.sln from the Visual Studio menus.
If you have not already done so, you also need to activate TR1 support.

You can find information about this topic in MSVC Help when entering TR1
in the search field. In short: it recommends to use the boost library.
So after you have downloaded boost, you simply can add the 
your_boost_root/boost/tr1 path to your global include directories to
get tr1 support.

