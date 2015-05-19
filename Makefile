################################################################################
# Makefile for Jetson TK1
################################################################################

SRCS = MyBlender.cpp \
	MyCompensator.cpp \
	MySeamFinder.cpp \
	MyStitcher.cpp \
	MyWarper.cpp \
	WebCam.cpp \
	Blender.cu \
	Compensator.cu \
	main.cpp

OBJS = \
	MyBlender.o \
	MyCompensator.o \
	MySeamFinder.o \
	MyStitcher.o \
	MyWarper.o \
	WebCam.o \
	main.o 

CUDA_OBJS = \
	Blender.o \
	Compensator.o

CFLAGS = -target-cpu-arch ARM
LFLAGS = -L/usr/local/lib

CUDA_CFLAGS = -c -O3 -target-cpu-arch ARM
CUDA_LFLAGS = -L/usr/local/cuda/include

NVCC = nvcc
GCC = arm-linux-gnueabihf-g++


.cu.o:
	$(NVCC) $(CUDA_CFLAGS) $<

.c.o:
	$(GCC) $(CFLAGS) $<
 
LIBS := -lcudart -lopencv_core -lopencv_gpu -lopencv_highgui -lopencv_imgproc -lopencv_stitching


all: StitchCam

StitchCam: $(OBJS) $(CUDA_OBJS)
	$(GCC) $(CUDA_LFLAGS) $(LFLAGS) -o "StitchCam" $(OBJS) $(CUDA_OBJS) $(LIBS)

clean:
	-$(RM) $(CU_DEPS)$(OBJS)$(C++_DEPS)$(C_DEPS)$(CC_DEPS)$(CPP_DEPS)$(EXECUTABLES)$(CXX_DEPS)$(C_UPPER_DEPS) StitchCam
	-@echo ' '

