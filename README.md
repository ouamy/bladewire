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
chmod u+x windows/build/compile.sh
./windows/build/compile.sh
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
