ARCH       := $(shell getconf LONG_BIT)
OS         := $(shell cat /etc/issue)

# Gets Driver Branch
DRIVER_BRANCH := $(shell nvidia-smi | grep Driver | cut -f 3 -d' ' | cut -f 1 -d '.')

# Location of the CUDA Toolkit
CUDA_PATH ?= "/usr/local/cuda"

ifeq (${ARCH},$(filter ${ARCH},32 64))
    # If correct architecture and libnvidia-ml library is not found 
    # within the environment, build using the stub library
    
    ifneq (,$(findstring Ubuntu,$(OS)))
        DEB := $(shell dpkg -l | grep cuda)
        ifneq (,$(findstring cuda, $(DEB)))
            NVML_LIB := /usr/lib/nvidia-$(DRIVER_BRANCH)
        else 
            NVML_LIB := /lib${ARCH}
        endif
    endif
else
    NVML_LIB := /usr/local/cuda/lib${ARCH}/stubs/
    $(info "libnvidia-ml.so.1" not found, using stub library.)
endif

ifneq (${ARCH},$(filter ${ARCH},32 64))
	$(error Unknown architecture!)
endif

CFLAGS  := -I /usr/local/cuda/include
LDFLAGS := -lnvidia-ml

all: wgpu
wgpu: wgpu.o
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@
clean:
	-@rm -f wgpu.o
	-@rm -f wgpu

