# Makefile - Wrapper de CMake para compilar con `make` desde micro_sd/
#
# Uso:
#   make            # configura (cmake) y compila (make -j4)
#   make -j4        # idem, en paralelo
#   make clean      # borra el directorio build/
#
# El PICO_SDK_PATH se detecta automaticamente (../pico-sdk relativo al repo
# o la variable de entorno PICO_SDK_PATH).

REPO_ROOT := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
BUILD_DIR := $(REPO_ROOT)/build
TARGET    := micro_sd
ELF       := $(BUILD_DIR)/src/$(TARGET).elf
UF2       := $(BUILD_DIR)/src/$(TARGET).uf2

# SDK: variable de entorno, hermano del REPO (../pico-sdk), o hermano del repo
# anidado (../../pico-sdk, layout /ruta/pico/pico_src + /ruta/pico/pico-sdk)
ifeq ($(PICO_SDK_PATH),)
  PICO_SDK_PATH := $(firstword $(wildcard $(REPO_ROOT)/../pico-sdk $(REPO_ROOT)/../../pico-sdk))
endif
export PICO_SDK_PATH

ifeq ($(PICO_SDK_PATH),)
$(warning PICO_SDK_PATH no definido y no existe ../pico-sdk; exporte PICO_SDK_PATH)
endif

.PHONY: all clean

all: $(ELF)

$(BUILD_DIR)/CMakeCache.txt:
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake ..

$(ELF): $(BUILD_DIR)/CMakeCache.txt
	$(MAKE) -C $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
	@echo "build/ eliminado"
