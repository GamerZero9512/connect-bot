FLAGS ?=

ifeq ($(OS),Windows_NT)
	TARGET ?= connect.exe
else
	TARGET ?= connect
endif

.PHONY: clean

all: $(TARGET)

$(TARGET): connect.c
	gcc -O2 connect.c -Wall -Wextra -Wpedantic -Werror -std=c89 $(FLAGS) -o $(TARGET)

debug: connect.c
	gcc -O0 -fno-builtin-optimizations -g3 -fsanitize=address,undefined,leak connect.c -Wall -Wextra -Wpedantic -Werror -std=c89 $(FLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)
