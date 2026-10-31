# Simulador de dificultad en el PC (sin placa).
#   make          -> compila y ejecuta (300 batallas por casilla)
#   make N=1000   -> mas batallas, numeros mas estables
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter
SRC_DIR  := ..
BUILD    := build
BIN      := $(BUILD)/sim
N        ?= 300

# battle_data.h incluye i18n.h, que pide Arduino.h: se usan los shims de los tests
SOURCES := sim.cpp $(SRC_DIR)/battle.cpp $(SRC_DIR)/gyms.cpp $(SRC_DIR)/i18n.cpp \
           $(SRC_DIR)/trainer.cpp $(SRC_DIR)/explore.cpp $(SRC_DIR)/rewards.cpp \
           $(SRC_DIR)/shop.cpp $(SRC_DIR)/tournament.cpp \
           $(SRC_DIR)/test/shim/shim.cpp

.PHONY: all run clean
all: run

$(BIN): $(SOURCES) $(SRC_DIR)/battle.h $(SRC_DIR)/gyms.h $(SRC_DIR)/battle_data.h $(SRC_DIR)/tournament.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -I $(SRC_DIR)/test/shim -o $@ $(SOURCES)

run: $(BIN)
	@./$(BIN) $(N)

clean:
	rm -rf $(BUILD)
