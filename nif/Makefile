ERLANG_PATH = $(shell erl -eval 'io:format("~s", [lists:concat([code:root_dir(), "/erts-", erlang:system_info(version), "/include"])])' -s init stop -noshell)

OUTPUT_LIBRARY=tree_sitter_nif.so

all: $(OUTPUT_LIBRARY)

clean: $(OUTPUT_LIBRARY)
	rm -f $(OUTPUT_LIBRARY)

$(OUTPUT_LIBRARY): tree_sitter_nif.c
	$(CC)  -undefined dynamic_lookup -dynamiclib -fPIC -shared -o $@ $< -I $(ERLANG_PATH) ../c_src/libtree-sitter.a

tree_sitter_nif.c:
