# BladeWire
A First Person Shooter Game where only the most wired survives.

## Install
```bash
sudo apt update
sudo apt install libxi-dev
sudo apt install libglm-dev
sudo apt install libglfw3-dev
sudo apt install libopenal-dev
sudo apt install libassimp-dev
```

## Compile
```bash
g++ main.cpp controller/game_controller.cpp view/renderer.cpp view/model.cpp view/animation/animation.cpp model/audio_manager.cpp view/glad/src/glad.cpp \
    -Iview/glad/include -Icontroller -Iview -Imodel \
    -o bladewire \
    -lglfw -ldl -lGL -lopenal -lX11 -lpthread -lXrandr -lXi -lassimp
```

## Run
```bash
./bladewire
```
