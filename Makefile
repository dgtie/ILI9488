PROG = ../prog/prog
BIN = $(HOME)/.platformio/packages/toolchain-microchippic32/bin
GPP = $(BIN)/pic32-g++
GCC = $(BIN)/pic32-gcc
BIN2HEX = $(BIN)/pic32-bin2hex
SIZE = $(BIN)/pic32-size

SRC = $(shell ls *.cpp)
CSRC = $(shell ls *.c)
OBJ = $(SRC:.cpp=.o) $(CSRC:.c=.o)
TARGET = out

CFLAGS = -mprocessor=32MX250F128D -Iinclude
LDFLAGS = -mprocessor=32MX250F128D -Wl,-Map=$(TARGET).map -T app.ld

all: $(TARGET).hex

$(TARGET).hex: $(TARGET).elf
	$(BIN2HEX) $<

$(TARGET).elf: $(OBJ)
	$(GPP) $(LDFLAGS) -o $@ $^
	$(SIZE) $(TARGET).elf

%.o: %.cpp
	$(GPP) $(CFLAGS) -o $@ -c $<

%.o: %.c
	$(GCC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf *.o $(TARGET).*

flash:
	$(PROG) $(TARGET).hex

test:
	minicom -D /dev/ttyACM0

