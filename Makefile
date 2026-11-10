CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -Iinclude
SRC_DIR = src
INC_DIR = include
OBJ_DIR = build
TEST_DIR = tests
BIN = statki

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Biblioteki sieciowe dla Windows vs POSIX
ifeq ($(OS),Windows_NT)
    LIBS = -lws2_32
    TARGET = $(BIN).exe
    TEST_GAME = test_game.exe
    TEST_UI = test_ui.exe
    TEST_AI = test_ai.exe
    TEST_SIM = test_ai_sim.exe
    TEST_PROTO = test_protocol.exe
    RM = del /Q /F
    MKDIR = if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)
else
    LIBS =
    TARGET = $(BIN)
    TEST_GAME = test_game
    TEST_UI = test_ui
    TEST_AI = test_ai
    TEST_SIM = test_ai_sim
    TEST_PROTO = test_protocol
    RM = rm -rf
    MKDIR = mkdir -p $(OBJ_DIR)
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(MKDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Cel do uruchamiania wszystkich testów jednostkowych i symulacji
test: $(TEST_GAME) $(TEST_UI) $(TEST_AI) $(TEST_SIM) $(TEST_PROTO)
	./$(TEST_GAME)
	./$(TEST_UI)
	./$(TEST_AI)
	./$(TEST_SIM)
	./$(TEST_PROTO)

$(TEST_GAME): $(SRC_DIR)/game.c $(TEST_DIR)/test_game.c
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_UI): $(SRC_DIR)/game.c $(SRC_DIR)/ui.c $(TEST_DIR)/test_ui.c
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_AI): $(SRC_DIR)/game.c $(SRC_DIR)/ai.c $(TEST_DIR)/test_ai.c
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_SIM): $(SRC_DIR)/game.c $(SRC_DIR)/ai.c $(TEST_DIR)/test_ai_simulation.c
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_PROTO): $(SRC_DIR)/protocol.c $(TEST_DIR)/test_protocol.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) $(OBJ_DIR) $(TARGET) $(TEST_GAME) $(TEST_UI) $(TEST_AI) $(TEST_SIM) $(TEST_PROTO)

.PHONY: all clean test
