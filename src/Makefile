# compiler
CC = gcc

# compiler flags
CFLAGS = -Wall -Wextra -g

# source files
SRCS = uart_defines.c ring_buffer.c packet.c

# object files
OBJS = $(SRCS:.c=.o)

# exe file
OUT = uart_driver

# default target
all: $(OUT)

# link all object files
$(OUT): $(OBJS)
	$(CC) $(OBJS) -o $(OUT)

# compile c and object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# clean generated files
clean:
	rm -f $(OBJS) $(OUT)

# do not create files for these names
.PHONY: all clean