# Introduction

Single C-file executable that displays custom swaybar in [SwayWM](https://swaywm.org/) (tiling Wayland Compositor)

# How to build

Just run following commands to build using [nob.h](https://github.com/tsoding/nob.h) (NoBuild system by Tsoding):

```bash
$> $(CC) -o nob nob.c
$> ./nob
```

# How to use

Just add following to your global SwayWM configuration

```
...
exec /path/to/swaystatus
...
```

That should be it. Enjoy!
