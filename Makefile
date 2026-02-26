ifeq ($(OS),Windows_NT)
	detected_OS := Windows
	PLATFORM_SRC = Exis/src/platform/platformwindows.cpp
	APPLICATION_SRC = Exis/src/platform/applicationwindows.cpp
	WINDOW_SRC = Exis/src/platform/glfwwindow.cpp
	APP_NAME := application.exe

    CFLAGS += -DPLATFORM_WINDOWS
	PLATFORM_LIBS = -lshell32 -lopengl32 -lglfw3
	SHARED_EXT = dll
	REMOVE = del
    COPY = copy
    COPY_TO = .

else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
		APP_NAME := application
        PLATFORM_SRC = Exis/src/platform/platformlinux.cpp
        CFLAGS += -DPLATFORM_LINUX
		PLATFORM_LIBS = -ldl -lGL -lglfw
		SHARED_EXT = so
		REMOVE = rm -f
        COPY = cp
    	COPY_TO = .
    endif
endif

#Compilation
CXX = clang++ -std=c++14
CXXFLAGS = --target=x86_64-pc-windows-msvc -m64 -W -Wall -Wno-missing-field-initializers -g -O0 -D_CRT_SECURE_NO_WARNINGS $(CFLAGS) #-march=native #-fno-fast-math # da provare a inserire nel caso si hanno dei problemi con i calcoli metematici 

# LDFLAGS = -lgame -lshell32 -lopengl32 -lglfw3 -Xlinker /subsystem:console
LIBS = -L Exis/external/libs/glfw -L Exis/external/libs/fmod -L Exis/external/libs/freetype
INCLUDE :=-I Exis/external/glfw/include -I Exis/external -I Exis/src -I Exis/external/fmod/core/inc 
INCLUDE_GAME :=-I game -I Exis/src -I Exis/external/ 

#Sources
GAME_SRC = \
	game/*.cpp \

APP_SRC = \
	Exis/src/application/application.cpp \
	

CORE_SRC = \
	Exis/src/core/arena.cpp \
	Exis/src/core/engine.cpp \
	Exis/src/core/audioengine.cpp \
	Exis/src/core/tracelog.cpp \
	Exis/src/core/ecs.cpp \
	Exis/src/core/input.cpp \
	Exis/src/core/profiler.cpp \
	Exis/src/core/camera.cpp \
	Exis/src/core/serialization.cpp \
	Exis/src/core/animationmanager.cpp \
	Exis/src/core/colliders.cpp \
	Exis/src/core/tilemap.cpp \
	Exis/src/core/ui.cpp \
	Exis/src/core/mystring.cpp \
	$(PLATFORM_SRC) \
	$(APPLICATION_SRC) \
	$(WINDOW_SRC) \

RENDERING_SRC = \
	Exis/src/renderer/shader.cpp \
	Exis/src/renderer/renderer.cpp \
	Exis/src/renderer/texture.cpp \
	Exis/src/renderer/fontmanager.cpp \

UTILITIES_SRC = \
	Exis/src/glad.c \

all: core.$(SHARED_EXT) game.$(SHARED_EXT) $(APP_NAME)
#game: game.dll
#core: core.dll
#kit: kit.dll

############################
# Compile glad.c as C
############################
Exis/src/glad.o: Exis/src/glad.c
	$(CXX) $(CXXFLAGS) -I Exis/external -c $< -o $@

ifeq ($(OS),Windows_NT)
copy_libs:
	@echo "Copying required libraries..."
	cmd /c copy /Y "Exis\\external\\libs\\glfw\\glfw3.dll" .
	cmd /c copy /Y "Exis\\external\\libs\\fmod\\fmodL.dll" .
else
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
copy_libs:
	@echo "Copying required libraries..."
	cp Exis/external/libs/glfw/libglfw.so .
	cp Exis/external/libs/fmod/libfmodL.so .
endif
endif

#NOTE: -lfmodL_vc is the debug version which print every error, just swap to -lfmod_vc for the realease build!!!
core.$(SHARED_EXT): ${CORE_SRC} ${RENDERING_SRC} Exis/src/glad.o
	@echo "Cleaning old core.dll"
	$(REMOVE) *.o
	$(REMOVE) core.$(SHARED_EXT)
	@echo "Building the core library"
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(LIBS) -lfreetype -lfmodL_vc -lglfw3 $(PLATFORM_LIBS) -DCORE_EXPORT -o $@ $^ -shared

game.$(SHARED_EXT): ${GAME_SRC} 
	$(REMOVE) game.pdb
	@echo "Building the game"
	$(CXX) $(CXXFLAGS) $(INCLUDE_GAME) $(LIBS) -lfreetype -DGAME_EXPORT -o $@ -lcore $^ -shared 
	@echo "Game builded successfull"
	

$(APP_NAME): ${APP_SRC} Exis/src/glad.o
	@echo "Building the application"
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(LIBS) -lfreetype $^ -o $@ $(PLATFORM_LIBS) -lcore
	$(MAKE) copy_libs
	@echo "System built successfully"
	@echo "System builded successfull"
	
clean:
	$(REMOVE) application application.exe
	$(REMOVE) *.dll *.so *.o *.pdb *.ilk 
	$(REMOVE) core.* game.* kit.*
	$(REMOVE) Exis\src\glad.o




#COMMAND
# ./build_web.bat
#WEB
# Server locally
# python -m http.server 808

# Open http://localhost:8080/game.html