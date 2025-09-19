# How to

##### Install dependencies

```shell
$ sudo dnf builddep plasma-desktop
```



##### Compile and install `kwin`

```shell
$ plasmashell --version
$ git checkout customize/v<x.y.z>
$ mkdir build
$ cd build
$ cmake ..
$ make -j$(nproc)
$ sudo make install
```





# Customization

##### Do not include background when caching



##### Include background when folder is dragged over



##### Add shortcuts

This is part of an effort to bring back the Mac OS 9 desktop environment



##### Fix desktop icon label text

This is part of an effort to bring back the Mac OS 9 desktop environment

