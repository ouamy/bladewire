# BladeWire
A First Person Shooter Game where only the most wired survives.

## Install
```bash
sudo apt update
sudo apt install -y libxi-dev
sudo apt install -y libglm-dev
sudo apt install -y libglfw3-dev
sudo apt install -y libopenal-dev
sudo apt install -y libassimp-dev
sudo apt install -y mingw-w64
sudo apt install -y libglfw3-dev-mingw-w64 libglm-dev-mingw-w64 libopenal-dev-mingw-w64
```

## Compile
### Linux
```bash
g++ main.cpp controller/game_controller.cpp view/renderer.cpp view/model.cpp view/animation/animation.cpp model/audio_manager.cpp view/glad/src/glad.cpp \
    -Iview/glad/include -Icontroller -Iview -Imodel \
    -o bladewire \
    -lglfw -ldl -lGL -lopenal -lX11 -lpthread -lXrandr -lXi -lassimp
```
### Windows
```bash
wget https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.bin.WIN64.zip -P bladewire/dependencies
unzip bladewire/dependencies/glfw-3.3.8.bin.WIN64.zip -d bladewire/dependencies/glfw-3.3.8.bin.WIN64

sudo cp -r bladewire/dependencies/glfw-3.3.8.bin.WIN64/glfw-3.3.8.bin.WIN64/include/GLFW /usr/x86_64-w64-mingw32/include/
sudo cp bladewire/dependencies/glfw-3.3.8.bin.WIN64/glfw-3.3.8.bin.WIN64/lib-mingw-w64/libglfw3.a /usr/x86_64-w64-mingw32/lib/

wget https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip -P bladewire/dependencies
unzip bladewire/dependencies/1.0.1.zip -d bladewire/dependencies/glm-1.0.1
sudo cp -r bladewire/dependencies/glm-1.0.1/glm-1.0.1/glm /usr/x86_64-w64-mingw32/include/

wget https://www.openal.org/openal_webstf/openal-sdk/openal-sdk-1.1.zip -P bladewire/dependencies
wget https://github.com/kcat/openal-soft/archive/refs/tags/1.23.1.zip -P bladewire/dependencies
unzip bladewire/dependencies/1.23.1.zip -d bladewire/dependencies/openal-soft-1.23.1

cd bladewire/dependencies/openal-soft-1.23.1 && mkdir -p build
cd bladewire/dependencies/openal-soft-1.23.1/build
cmake -DCMAKE_TOOLCHAIN_FILE=/usr/share/cmake-mingw-w64/toolchain-x86_64-w64-mingw32.cmake -DCMAKE_INSTALL_PREFIX=/usr/x86_64-w64-mingw32 -DALSOFT_BUILD_EXAMPLES=OFF -DALSOFT_BUILD_TESTS=OFF -DALSOFT_INSTALL_PKGCONFIG=OFF -DALSOFT_NO_HRTF_DEFAULTS=ON ..
make
sudo make install

cd ~
wget https://github.com/assimp/assimp/archive/refs/tags/v5.3.1.zip -P bladewire/dependencies
unzip bladewire/dependencies/v5.3.1.zip -d bladewire/dependencies/assimp-5.3.1

cd bladewire/dependencies/assimp-5.3.1 && mkdir -p build
cd bladewire/dependencies/assimp-5.3.1/build
cmake -DCMAKE_TOOLCHAIN_FILE=/usr/share/cmake-mingw-w64/toolchain-x86_64-w64-mingw32.cmake -DCMAKE_INSTALL_PREFIX=/usr/x86_64-w64-mingw32 -DASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT=ON -DASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT=ON -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_TESTS=OFF -DASSIMP_BUILD_SAMPLES=OFF ..
make
sudo make install

cd ~
cp -r bladewire/windows/libs bladewire/dependencies/openal-soft-1.23.1/
cd bladewire

x86_64-w64-mingw32-g++ main.cpp controller/game_controller.cpp view/renderer.cpp view/model.cpp view/animation/animation.cpp model/audio_manager.cpp view/glad/src/glad.cpp \
-Iview/glad/include -Icontroller -Iview -Imodel \
-o bladewire.exe \
-L/usr/x86_64-w64-0mingw32/lib -L/home/user0/bladewire/dependencies/openal-soft-1.23.1/libs/Win64 \
-lglfw3 -lopengl32 -lOpenAL32 -lassimp -lgdi32 -lwinmm -lws2_32 \
-std=c++17 -DGLM_ENABLE_EXPERIMENTAL

mv dependencies windows

mv bladewire.exe windows/build

rm windows/dependencies/1.0.1.zip
rm windows/dependencies/1.23.1.zip
rm windows/dependencies/glfw-3.3.8.bin.WIN64.zip
rm windows/dependencies/v5.3.1.zip
```

## Run
### Linux
```bash
./bladewire
```
### Windows
```bash
windows/build/bladewire.exe
```
