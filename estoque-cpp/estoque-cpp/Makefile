# ============================================================================
#  Makefile - Sistema de Estoque (C++17)
#
#  make               compila o programa           (build/estoque)
#  make executar      compila e executa
#  make testes        compila e roda os testes de unidade
#  make cobertura     roda os testes com gcov e mostra a cobertura de linhas
#  make sanitizar     roda os testes com AddressSanitizer + UBSan
#  make doc           gera a documentação com Doxygen (docs/doxygen/html)
#  make demo          executa uma sessão simulada a partir de exemplos/entrada_demo.txt
#  make limpar        apaga os arquivos gerados
#
#  (aliases em inglês: run, test, coverage, clean)
# ============================================================================

CXX      ?= g++
PADRAO   := -std=c++17
AVISOS   := -Wall -Wextra -Wpedantic
OTIMIZA  ?= -O2
INCLUDES := -Iinclude
DEPFLAGS  = -MMD -MP

BUILD := build

FONTES_LIB   := $(filter-out src/main.cpp,$(wildcard src/*.cpp))
FONTES_TESTE := $(wildcard tests/*.cpp)

# --- build normal -----------------------------------------------------------
OBJ_LIB   := $(patsubst src/%.cpp,$(BUILD)/obj/%.o,$(FONTES_LIB))
OBJ_MAIN  := $(BUILD)/obj/main.o
OBJ_TESTE := $(patsubst tests/%.cpp,$(BUILD)/obj_teste/%.o,$(FONTES_TESTE))
APP       := $(BUILD)/estoque
TESTES    := $(BUILD)/testes

# --- build de cobertura (sem otimização, com instrumentação do gcov) ----------
COV_DIR       := $(BUILD)/cobertura
COV_LIB       := $(patsubst src/%.cpp,$(COV_DIR)/lib/%.o,$(FONTES_LIB))
COV_TESTE     := $(patsubst tests/%.cpp,$(COV_DIR)/teste/%.o,$(FONTES_TESTE))
TESTES_COV    := $(BUILD)/testes_cobertura

# --- build com sanitizers -----------------------------------------------------
SAN_DIR    := $(BUILD)/sanitizar
SAN_FLAGS  := -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
SAN_LIB    := $(patsubst src/%.cpp,$(SAN_DIR)/lib/%.o,$(FONTES_LIB))
SAN_TESTE  := $(patsubst tests/%.cpp,$(SAN_DIR)/teste/%.o,$(FONTES_TESTE))
TESTES_SAN := $(BUILD)/testes_sanitizar

.PHONY: all executar run testes test cobertura coverage sanitizar doc demo limpar clean ajuda

all: $(APP)

ajuda:
	@sed -n '2,15p' Makefile

# ----------------------------------------------------------------------------
$(APP): $(OBJ_LIB) $(OBJ_MAIN)
	$(CXX) $^ -o $@

$(BUILD)/obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(PADRAO) $(AVISOS) $(OTIMIZA) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

$(TESTES): $(OBJ_LIB) $(OBJ_TESTE)
	$(CXX) $^ -o $@

$(BUILD)/obj_teste/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(PADRAO) $(AVISOS) $(OTIMIZA) $(INCLUDES) -Itests $(DEPFLAGS) -c $< -o $@

executar: $(APP)
	./$(APP)
run: executar

testes: $(TESTES)
	./$(TESTES)
test: testes

# ----------------------------------------------------------------------------
$(COV_DIR)/lib/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(PADRAO) $(AVISOS) -O0 -g --coverage $(INCLUDES) $(DEPFLAGS) -c $< -o $@

$(COV_DIR)/teste/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(PADRAO) $(AVISOS) -O0 -g --coverage $(INCLUDES) -Itests $(DEPFLAGS) -c $< -o $@

$(TESTES_COV): $(COV_LIB) $(COV_TESTE)
	$(CXX) --coverage $^ -o $@

cobertura: $(TESTES_COV)
	@find $(COV_DIR) -name '*.gcda' -delete
	./$(TESTES_COV)
	@echo
	@echo "=== cobertura de linhas por arquivo (src/) ==="
	@for f in $(FONTES_LIB); do gcov -n -o $(COV_DIR)/lib $$f 2>/dev/null; done | \
	awk '/^File /{arq=$$2; gsub(/[^A-Za-z0-9_\/.+-]/,"",arq); if (arq !~ /^src\//) arq=""} \
	     /^Lines executed:/ && arq!="" { split($$0,p,"[:% ]+"); pct=p[3]; n=$$NF; \
	        if (!(arq in visto)) { visto[arq]=1; printf "  %-24s %6.2f%% de %d linhas\n", arq, pct, n; exec+=pct*n/100; tot+=n } arq="" } \
	     END { if (tot>0) printf "  %-24s %6.2f%% de %d linhas\n", "TOTAL", 100*exec/tot, tot }'
coverage: cobertura

# ----------------------------------------------------------------------------
$(SAN_DIR)/lib/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(PADRAO) $(AVISOS) $(SAN_FLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

$(SAN_DIR)/teste/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(PADRAO) $(AVISOS) $(SAN_FLAGS) $(INCLUDES) -Itests $(DEPFLAGS) -c $< -o $@

$(TESTES_SAN): $(SAN_LIB) $(SAN_TESTE)
	$(CXX) $(SAN_FLAGS) $^ -o $@

sanitizar: $(TESTES_SAN)
	./$(TESTES_SAN)

# ----------------------------------------------------------------------------
doc:
	doxygen Doxyfile
	@echo "documentação gerada em docs/doxygen/html/index.html"

demo: $(APP)
	rm -rf $(BUILD)/dados_demo
	./$(APP) $(BUILD)/dados_demo < exemplos/entrada_demo.txt

limpar:
	rm -rf $(BUILD) docs/doxygen
clean: limpar

-include $(wildcard $(BUILD)/obj/*.d) $(wildcard $(BUILD)/obj_teste/*.d) \
         $(wildcard $(COV_DIR)/*/*.d) $(wildcard $(SAN_DIR)/*/*.d)
