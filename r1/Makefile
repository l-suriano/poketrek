# Version de PokeTrek para el Rabbit R1 (una "creation" web de 240x282).
#   make test   -> prueba nativa del puente (g++), sin Emscripten
#   make web    -> compila el motor a WebAssembly en web/poketrek.js (necesita emcc)
#   make serve  -> sirve web/ en http://localhost:8000 para probar en el navegador
SRC_DIR := ..
CORE := $(SRC_DIR)/battle.cpp $(SRC_DIR)/gyms.cpp $(SRC_DIR)/trainer.cpp $(SRC_DIR)/explore.cpp \
        $(SRC_DIR)/rewards.cpp $(SRC_DIR)/shop.cpp $(SRC_DIR)/tournament.cpp $(SRC_DIR)/i18n.cpp \
        $(SRC_DIR)/test/shim/shim.cpp
INC := -I $(SRC_DIR)/test/shim
BUILD := build

CXX ?= g++
EMCC ?= em++
EXPORTS := $(shell grep '^API' bridge.cpp | grep -o 'pt_[a-z0-9_]*' | sort -u | sed 's/^/_/' | paste -sd, -)

.PHONY: test web serve lib dev clean
test: $(BUILD)/test_bridge
	./$(BUILD)/test_bridge $(BUILD)/bridge_out.txt
	python3 check_json.py $(BUILD)/bridge_out.txt

$(BUILD)/test_bridge: bridge.cpp test_bridge.cpp $(CORE)
	@mkdir -p $(BUILD)
	$(CXX) -std=c++17 -O1 -g -Wall -Wextra -Wno-unused-parameter $(INC) -o $@ bridge.cpp test_bridge.cpp $(CORE)

# un solo .js con el .wasm dentro: un archivo menos que cachear en el R1
web: web/poketrek.js
web/poketrek.js: bridge.cpp $(CORE)
	$(EMCC) -std=c++17 -O2 $(INC) bridge.cpp $(CORE) -o $@ \
	  -sMODULARIZE=1 -sEXPORT_NAME=PokeTrekModule -sSINGLE_FILE=1 -sENVIRONMENT=web \
	  -sALLOW_MEMORY_GROWTH=1 -sEXPORTED_RUNTIME_METHODS=ccall,UTF8ToString,HEAPU8 \
	  -sEXPORTED_FUNCTIONS=$(EXPORTS),_malloc,_free

serve:
	cd web && python3 -m http.server 8000

# puente nativo como biblioteca para tools/dev_server.py (probar la interfaz sin Emscripten)
LIBEXT := $(if $(filter Windows_NT,$(OS)),dll,so)
lib: $(BUILD)/bridge.$(LIBEXT)
$(BUILD)/bridge.$(LIBEXT): bridge.cpp $(CORE)
	@mkdir -p $(BUILD)
	$(CXX) -std=c++17 -O1 -shared -fPIC $(INC) -o $@ bridge.cpp $(CORE) $(if $(filter dll,$(LIBEXT)),-static-libgcc -static-libstdc++,)

dev: lib
	python3 tools/dev_server.py 8000

clean:
	rm -rf $(BUILD)
