# Intended to be run via elixir_make

ERL_CFLAGS ?= -I$(ERL_EI_INCLUDE_DIR)
ERL_LDFLAGS ?= -L$(ERL_EI_LIBDIR)

OUTPUT_LIBRARY=tree_sitter_nif.so

ifeq ($(OS),Windows_NT)
	$(error "Windows is not supported")
else ifeq ($(shell uname),Darwin)
	# not linux, so use MacOS shared library ld flags
	LDFLAGS += -undefined dynamic_lookup -dynamiclib -fPIC -shared
else
	# linux and other platforms use `shared` instead of `dynamiclib`
	LDFLAGS += -fPIC -shared
endif

all: $(OUTPUT_LIBRARY)

clean:
	rm -f $(PREFIX)/$(OUTPUT_LIBRARY)

$(OUTPUT_LIBRARY): tree_sitter_nif.c
	$(CC) $(CFLAGS) $(ERL_CFLAGS) $(LDFLAGS) $(ERL_LDFLAGS) -I ../c_src/lib/include -o $@ $< ../c_src/libtree-sitter.a

tree_sitter_nif.c:

.PHONY: all clean
