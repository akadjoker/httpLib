
TARGET = main

CC = clang
CFLAGS = -Wall -Wextra -g -fsanitize=address
SOURCES = main.c
OBJECTS = $(SOURCES:.c=.o)
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) 
	@rm -f $(OBJECTS)
	@./$(TARGET)


%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(OBJECTS) $(TARGET)
