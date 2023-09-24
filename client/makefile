
TARGET = main


CC = gcc
CFLAGS = -Wall -g -fsanitize=address
SOURCES = client.c
OBJECTS = $(SOURCES:.c=.o)
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) -lraylib -lpthread -lm
	@rm -f $(OBJECTS)
	@./$(TARGET)


%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(OBJECTS) $(TARGET)
