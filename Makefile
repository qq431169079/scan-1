CFLAGS = -I.
TARGET = scanner
DEPS = utils.h scanner.h

.PHONY: all scanner clean
all: $(TARGET)
$(TARGET): main.o
	$(CC) -o $@ $^

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	$(RM) *.o *~ a.out $(TARGET)