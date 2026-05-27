PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin

FLAGS ?=
CC ?= cc

ifeq ($(OS),Windows_NT)
	TARGET ?= connect.exe
else
	TARGET ?= connect
endif

.PHONY: all debug clean install

all: $(TARGET)

$(TARGET): connect.c
	$(CC) -O2 connect.c -Wall -Wextra -pedantic -std=c89 $(FLAGS) -o $(TARGET)

debug: connect.c
	$(CC) -O0 -fno-builtin-optimizations -g3 -fsanitize=address,undefined,leak connect.c -Wall -Wextra -pedantic -Werror -std=c89 $(FLAGS) -o $(TARGET)

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm -f $(TARGET)
