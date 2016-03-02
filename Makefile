NAME = Test

DEFINES =

C_OPTS =	-std=c99 \
			-g \
			-O2 \
			-Werror

LIBS = 

SOURCE_DIR = .
BUILD_DIR = Build

.PHONY: all
all: $(NAME) README.md

C_FILES =	Test.c \
			SmallJSONParser.c

C_OBJS = $(C_FILES:%.c=$(BUILD_DIR)/%.o)

ALL_CFLAGS = $(C_OPTS) $(DEFINES) $(CFLAGS)
ALL_LDFLAGS = $(LD_FLAGS)

AUTODEPENDENCY_CFLAGS=-MMD -MF$(@:.o=.d) -MT$@

.SUFFIXES: .o .c

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(ALL_CFLAGS) $(AUTODEPENDENCY_CFLAGS) -c $< -o $@

$(NAME): $(C_OBJS)
	$(CC) $(ALL_LDFLAGS) -o $@ $^ $(LIBS)

README.md: Tools/SimplestDocumentation.pl SmallJSONParser.h
	perl $^ $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(NAME)

-include $(C_OBJS:.o=.d)

