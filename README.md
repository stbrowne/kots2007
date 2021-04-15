# KOTS2007

This contains an initial import of the KOTS2007 source code as well as a few instructions to get things going. Note that this documentation is pretty old and basic and will probably need to be updated in the future.

## Installation
- Download the binaries for your platform
- Extract them to your Quake 2 folder so that gamex86 goes into a subfolder named `kots2007`
- Windows: Download the pre-reqs which includes dependent libraries (libcurl, libmysqlclient, pthreads)
- Linux: Ensure library dependencies are installed or compile and install them (libcurl, libmysqlclient, pthreads)

## Database
See [kots_db.sql](kots_db.sql). However, sadly if I remember correctly there's a syntax error in there somewhere that may have to be corrected before it can be imported. It's also extremely old and may not import directly to newer mysql versions anyway.

## Compiling

### Windows
- Clone the repo including submodules
- See the mysql section below about extracting libmysqlclient to your source folder

### Linux
TODO

## MySQL
https://downloads.mysql.com/archives/get/p/23/file/mysql-noinstall-5.0.41-win32.zip

The previous releases didn't build mysql from source and instead used the noinstall link above. This may change in the future, but for now you just need to create a mysql folder in the source root and extract the include and lib folders into it for compiling.

## Linux notes
The use of the Linux version requires the mysql shared library be compiled on the box. I used version 5.0.45 for testing. The easiest way to get the mysql shared library is to download a precompiled version or an RPM with the precompiled version.

http://www.mysql.org/downloads/mysql/5.0.html#downloads
Just find your correct distro and download the one that says "Shared Libraries." If for some reason you can't install the RPM then you can use one of the non-RPM packages and configure and make it.

MySQL Shared library v5.0.45
http://www.ihasacrayon.net/kots/downloads/libmysqlclient.so.15.gz
Alternatively you can also simply copy the above mysql shared library file to /usr/lib/ or /lib/. If you don't have access to these paths you can also put it in the Quake 2 directory or the mod folder, but you need to make sure you add the path you put it in to LD_LIBRARY_PATH if you do this every time you start Quake 2. For example:
```
export LD_LIBRARY_PATH=/home/user/quake2:$LD_LIBRARY_PATH
/home/user/quake2/r1q2ded +game kots2007 +exec server.cfg
```

### Compilation notes
- Kernel Version: 2.4.22
- GCC: 3.3.2
- GLibC: 2.3.2