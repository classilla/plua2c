# Plua2c

Plua is a superset of Lua for classic PalmOS devices with added support for graphics, UI, networking, events and sound. This is the source code for the Plua2c "cross-compiler" that generates .prc
files from Plua 2.0 source. You still need Plua2RT and MathLib to run the .prc
installed on the same device, which can be downloaded from
[http://www.floodgap.com/retrotech/plua/](Plua Revisited).

Plua2c has generously been relicensed under the same MIT terms as Lua by
its original author, Marcio Migueletto de Andrade. For details, see
`LICENSE.txt`.

The current version generates binaries compatible with Plua2RT 2.0, based on
Lua 5.0.3. It should build and run on 32- and 64-bit hosts, either big or
little endian. It has been tested on Linux and Mac OS X (PowerPC and x86_64).

Most systems will build with a simple `make`. This includes current versions
of macOS. PowerPC-based versions of Mac OS X should use
`make -f Makefile.macos`.

If you are building for a foreign platform, edit `Makefile.cross` if necessary,
which is in three places in this tree, and use that instead
(`make -f Makefile.cross`).
