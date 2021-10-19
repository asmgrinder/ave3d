# ave3d
Transparency and PBR (fork of https://github.com/Nadrin/PBR, OpenGL 4.5 only)

# Run
To run program you only need 'data' folder.

### Controls

Input        | Action
-------------|-------
LMB drag     | Rotate camera
RMB drag     | Rotate 3D model
Scroll wheel | Zoom in/out
F1-F3        | Toggle analytical lights on/off

# Build

git clone --recursive https://github.com/asmgrinder/ave3d.git

cd ave3d/data

cmake -S .. -B ../build

cmake --build ../build -- -jN   # N is number of cores to use for building

cp ../build/ave3d.exe .

ave3d.exe

CMake GUI can be used to turn off assimp and glfw install check boxes and test examples build

## Third party libraries

This project makes use of the following open source libraries:

- [Open Asset Import Library](http://assimp.sourceforge.net/)
- [stb_image](https://github.com/nothings/stb)
- [GLFW](http://www.glfw.org/)
- [GLM](https://glm.g-truc.net/)
- [glad](https://github.com/Dav1dde/glad) (used to generate OpenGL function loader)

## Included assets

The following assets are bundled with the project:

- "Cerberus" gun model by [Andrew Maximov](http://artisaverb.info).
- HDR environment map by [Bob Groothuis](http://www.bobgroothuis.com/blog/) obtained from [HDRLabs sIBL archive](http://www.hdrlabs.com/sibl/archive.html) (distributed under [CC-BY-NC-SA 3.0](https://creativecommons.org/licenses/by-nc-sa/3.0/us/)).
