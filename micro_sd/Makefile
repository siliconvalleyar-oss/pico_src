# Makefile - Wrapper de CMake para compilar con `make` desde micro_sd/
#
# Uso:
#   make            # configura (cmake) y compila (make -j4)
#   make -j4        # idem, en paralelo
#   make clean      # borra el directorio build/
#
# Nota: delega SIEMPRE en el make generado por CMake, que rastrea las
# dependencias de los fuentes. (No depender del ELF como target: haria que
# "make" ignore cambios en los .c/.cpp despues del primer build.)
#
# El PICO_SDK_PATH se detecta automaticamente (../pico-sdk relativo al repo
# o la variable de entorno PICO_SDK_PATH).

REPO_ROOT := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
BUILD_DIR := $(REPO_ROOT)/build
TARGET    := micro_sd
ELF       := $(BUILD_DIR)/src/$(TARGET).elf

# Numero de jobs: heredar -j de MAKEFLAGS; si no viene, usar 4
ifneq (,$(findstring -j,$(MAKEFLAGS)))
  JOBS :=
else
  JOBS := -j4
endif

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

all:
	@mkdir -p $(BUILD_DIR)
	@if [ ! -f "$(BUILD_DIR)/CMakeCache.txt" ]; then \
	    echo "=== Configurando (cmake) ==="; \
	    cd $(BUILD_DIR) && cmake .. || exit 1; \
	fi
	@echo "=== Compilando $(TARGET) ==="
	$(MAKE) $(JOBS) -C $(BUILD_DIR)
	@if [ -f "$(ELF)" ]; then \
	    echo "=== OK: $(ELF) ==="; \
	else \
	    echo "=== ERROR: no se genero $(ELF) ===" >&2; exit 1; \
	fi

clean:
	rm -rf $(BUILD_DIR)
	@echo "build/ eliminado"
