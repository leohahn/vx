CPP      = g++
CPPFLAGS = -Wall -Wextra -std=c++14 \
           -I/usr/include -I./include -I/usr/local/include -I./src/dependencies -I./src/dependencies/glm
LDFLAGS  =

LIBS       = -lm -lGL -lglfw -lGLEW -lfreeimage

SRC      = src/main.cpp src/um.cpp src/vx_math.cpp src/um_image.cpp \
		   src/vx.cpp src/vx_shader_manager.cpp src/vx_string_hashmap.cpp src/vx_camera.cpp \
		   src/vx_log_manager.cpp src/vx_files.cpp src/vx_ui_manager.cpp src/vx_chunk_manager.cpp \
		   src/vx_frustum.cpp src/vx_depth_buffer.cpp src/dependencies/open-simplex-noise.cpp \
		   src/vx_depth_buffer_rasterizer.cpp

OBJ      = ${SRC:src/%.cpp=build/%.o}

EXE      = vx

build/configure: CPP = cc_args.py g++
build/configure: all

all: ${EXE}

build/%.o: src/%.cpp
	@echo CPP $< ==> $@
	@${CPP} -c ${CPPFLAGS} $< -o $@

${EXE}: ${OBJ}
	@${CPP} -o build/$@ ${OBJ} ${LIBS} ${LDFLAGS}

run: all
	@./build/${EXE}

clean:
	@echo cleaning: ${EXE} and .objs
	@rm ${OBJ}
	@rm ./build/${EXE}
