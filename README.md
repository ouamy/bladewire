# BladeWire
A First Person Shooter Game where only the most wired survives.

## Compile
```bash
sudo apt-get install libopenal-dev
sudo apt-get install libassimp-dev
```
```bash
g++ main.cpp controller/game_controller.cpp view/renderer.cpp view/model.cpp model/audio_manager.cpp glad/src/glad.cpp \
    -Iglad/include -Icontroller -Iview -Imodel \
    -o bladewire \
    -lglfw -ldl -lGL -lopenal -lX11 -lpthread -lXrandr -lXi -lassimp
```

## Run
```bash
./bladewire
```