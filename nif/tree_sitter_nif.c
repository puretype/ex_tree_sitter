#include <assert.h>
#include <dlfcn.h>
#include <erl_nif.h>
#include <string.h>
#include <tree_sitter/api.h>

// hello

#define MAX_PATHNAME 512
#define MAX_FUNCTIONNAME 128

static ErlNifResourceType* parser_type = NULL;
static ErlNifResourceType* language_type = NULL;
static ErlNifResourceType* query_type = NULL;
static ErlNifResourceType* tree_type = NULL;

typedef struct {
  TSParser* parser;
} parser_resource_t;

typedef struct {
  void* dl_handle;
  const TSLanguage* (*dl_function)();
} language_resource_t;

typedef struct {
  TSQuery* query;
} query_resource_t;

typedef struct {
  TSTree* tree;
} tree_resource_t;

ERL_NIF_TERM make_atom(ErlNifEnv* env, const char* atom_name) {
  ERL_NIF_TERM atom;

  if (enif_make_existing_atom(env, atom_name, &atom, ERL_NIF_LATIN1)) {
    return atom;
  }

  return enif_make_atom(env, atom_name);
}

ERL_NIF_TERM make_error_tuple(ErlNifEnv* env, const char* reason) {
    return enif_make_tuple2(env, make_atom(env, "error"), make_atom(env, reason));
}

//

ERL_NIF_TERM point_to_map(ErlNifEnv* env, const TSPoint point) {
  ERL_NIF_TERM point_map = enif_make_new_map(env);

  enif_make_map_put(env, point_map, make_atom(env, "row"), enif_make_int(env, point.row), &point_map);
  enif_make_map_put(env, point_map, make_atom(env, "column"), enif_make_int(env, point.column), &point_map);

  return point_map;
}

ERL_NIF_TERM query_error_to_atom(ErlNifEnv* env, TSQueryError error) {
  switch (error) {
    case TSQueryErrorNone:
      return make_atom(env, "none");
    case TSQueryErrorSyntax:
      return make_atom(env, "syntax");
    case TSQueryErrorNodeType:
      return make_atom(env, "node_type");
    case TSQueryErrorField:
      return make_atom(env, "field");
    case TSQueryErrorCapture:
      return make_atom(env, "capture");
    default:
      return make_atom(env, "unknown");
  }
}

//

ERL_NIF_TERM ts_new(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
  parser_resource_t* parser_resource = enif_alloc_resource(parser_type, sizeof(parser_resource_t));

  parser_resource->parser = ts_parser_new();
  ERL_NIF_TERM result = enif_make_resource(env, parser_resource);
  enif_release_resource(parser_resource);

  return result;
}

ERL_NIF_TERM ts_new_language(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 2) {
    return enif_make_badarg(env);
  }

  char path[MAX_PATHNAME];
  char function[MAX_FUNCTIONNAME];

  int path_size = enif_get_string(env, argv[0], path, MAX_PATHNAME, ERL_NIF_LATIN1);
  if (path_size <= 0) {
    return make_error_tuple(env, "bad_path_name");
  }

  int function_size = enif_get_string(env, argv[1], function, MAX_FUNCTIONNAME, ERL_NIF_LATIN1);
  if (function_size <= 0) {
    return make_error_tuple(env, "bad_function_name");
  }

  language_resource_t* language_resource = enif_alloc_resource(language_type, sizeof(language_resource_t));

  language_resource->dl_handle = dlopen(path, RTLD_LAZY);
  if (language_resource->dl_handle == NULL) {
    return make_error_tuple(env, dlerror()); // bad_library
  }

  language_resource->dl_function = dlsym(language_resource->dl_handle, function);
  if (language_resource->dl_function == NULL) {
    return make_error_tuple(env, "bad_function");
  }

  ERL_NIF_TERM result = enif_make_resource(env, language_resource);
  enif_release_resource(language_resource);

  return result;
}

ERL_NIF_TERM ts_set_language(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 2) {
    return enif_make_badarg(env);
  }

  parser_resource_t* parser_resource;
  if (!enif_get_resource(env, argv[0], parser_type, (void**)&parser_resource)) {
    return enif_make_badarg(env);
  }

  language_resource_t* language_resource;
  if (!enif_get_resource(env, argv[1], language_type, (void**)&language_resource)) {
    return enif_make_badarg(env);
  }

  if (ts_parser_set_language(parser_resource->parser, language_resource->dl_function())) {
    return make_atom(env, "ok");
  } else {
    return make_error_tuple(env, "version_mismatch");
  }
}

ERL_NIF_TERM ts_parse_string(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 2) {
    return enif_make_badarg(env);
  }

  parser_resource_t* parser_resource;
  if (!enif_get_resource(env, argv[0], parser_type, (void**)&parser_resource)) {
    return enif_make_badarg(env);
  }

  ErlNifBinary source_code;
  if (!enif_inspect_binary(env, argv[1], &source_code)) {
    return enif_make_badarg(env);
  }

  TSTree* tree = ts_parser_parse_string(parser_resource->parser, NULL, source_code.data, source_code.size);
  if (tree == NULL) {
    return make_error_tuple(env, "general_parse_error");
  }

  tree_resource_t* tree_resource = enif_alloc_resource(tree_type, sizeof(tree_resource_t));
  tree_resource->tree = tree;

  ERL_NIF_TERM result = enif_make_resource(env, tree_resource);
  enif_release_resource(tree_resource);

  return result;
}

ERL_NIF_TERM ts_new_query(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
  ErlNifBinary query_string;

  language_resource_t* language_resource;
  if (!enif_get_resource(env, argv[0], language_type, (void**)&language_resource)) {
    return enif_make_badarg(env);
  }

  if (!enif_inspect_binary(env, argv[1], &query_string)) {
    return enif_make_badarg(env);
  }

  uint32_t error_offset;
  TSQueryError error_type;

  TSQuery* query = ts_query_new(
    language_resource->dl_function(),
    query_string.data,
    query_string.size,
    &error_offset,
    &error_type
    );

  if (query == NULL) {
    return enif_make_tuple3(
      env,
      make_atom(env, "error"),
      query_error_to_atom(env, error_type),
      enif_make_int(env, error_offset)
      );
  }

  query_resource_t* query_resource = enif_alloc_resource(query_type, sizeof(query_resource_t));
  query_resource->query = query;

  ERL_NIF_TERM result = enif_make_resource(env, query_resource);
  enif_release_resource(query_resource);

  return enif_make_tuple2(env, make_atom(env, "ok"), result);
}

ERL_NIF_TERM ts_exec_query(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
  if (argc != 2) {
    return enif_make_badarg(env);
  }

  query_resource_t* query_resource;
  if (!enif_get_resource(env, argv[0], query_type, (void**)&query_resource)) {
    return enif_make_badarg(env);
  }

  const TSQuery* query = query_resource->query;

  tree_resource_t* tree_resource;
  if (!enif_get_resource(env, argv[1], tree_type, (void**)&tree_resource)) {
    return enif_make_badarg(env);
  }

  uint32_t capture_count = ts_query_capture_count(query);
  ERL_NIF_TERM capture_atoms[capture_count];

  for (uint32_t i = 0; i < capture_count; i++) {
    uint32_t length;
    const char* name = ts_query_capture_name_for_id(query, i, &length);
    capture_atoms[i] = enif_make_atom_len(env, name, length);
  }

  TSQueryCursor* cursor = ts_query_cursor_new();
  TSNode root = ts_tree_root_node(tree_resource->tree);

  ts_query_cursor_exec(cursor, query, root);
  
  ERL_NIF_TERM matches = enif_make_list(env, 0);
  TSQueryMatch match;
  while (ts_query_cursor_next_match(cursor, &match)) {
    ERL_NIF_TERM captures = enif_make_list(env, 0);
    for (uint32_t i = 0; i < match.capture_count; i++) {
      const TSQueryCapture* capture = &match.captures[i];
      ERL_NIF_TERM capture_item = enif_make_new_map(env);

      const char* node_type = ts_node_type(capture->node);
      ERL_NIF_TERM type = make_atom(env, node_type);

      enif_make_map_put(env, capture_item, make_atom(env, "start_byte"), enif_make_int(env, ts_node_start_byte(capture->node)), &capture_item);
      enif_make_map_put(env, capture_item, make_atom(env, "end_byte"), enif_make_int(env, ts_node_end_byte(capture->node)), &capture_item);
      enif_make_map_put(env, capture_item, make_atom(env, "start"), point_to_map(env, ts_node_start_point(capture->node)), &capture_item);
      enif_make_map_put(env, capture_item, make_atom(env, "end"), point_to_map(env, ts_node_end_point(capture->node)), &capture_item);
      enif_make_map_put(env, capture_item, make_atom(env, "type"), type, &capture_item);

      captures = enif_make_list_cell(env, enif_make_tuple2(env, capture_atoms[capture->index], capture_item), captures);
      // make list from array instead - since we know count ahead of time
    }

    matches = enif_make_list_cell(env, captures, matches);
  }

  ERL_NIF_TERM reversed_matches;
  enif_make_reverse_list(env, matches, &reversed_matches);

  ts_query_cursor_delete(cursor);

  return enif_make_tuple2(env, make_atom(env, "ok"), reversed_matches);
}

static void parser_type_destructor(ErlNifEnv* env, void* arg) {
  parser_resource_t* parser_resource = (parser_resource_t*)arg;

  if (parser_resource != NULL) {
    ts_parser_delete(parser_resource->parser);
  }
}

static void language_type_destructor(ErlNifEnv* env, void* arg) {
  language_resource_t* language_resource = (language_resource_t*)arg;

  if (language_resource != NULL) {
    dlclose(language_resource->dl_handle);
  }
}

static void query_type_destructor(ErlNifEnv* env, void* arg) {
  query_resource_t* query_resource = (query_resource_t*)arg;

  if (query_resource != NULL) {
    ts_query_delete(query_resource->query);
  }
}

static void tree_type_destructor(ErlNifEnv* env, void* arg) {
  tree_resource_t* tree_resource = (tree_resource_t*)arg;

  if (tree_resource != NULL) {
    ts_tree_delete(tree_resource->tree);
  }
}

///

static int on_load(ErlNifEnv* env, void** priv, ERL_NIF_TERM info) {

  parser_type = enif_open_resource_type(
      env,
      NULL,
      "ts_parser",
      parser_type_destructor,
      ERL_NIF_RT_CREATE,
      NULL);
  if (parser_type == NULL)
  {
    return -1;
  }

  language_type = enif_open_resource_type(
      env,
      NULL,
      "ts_language",
      language_type_destructor,
      ERL_NIF_RT_CREATE,
      NULL);
  if (language_type == NULL)
  {
    return -1;
  }

  query_type = enif_open_resource_type(
      env,
      NULL,
      "ts_query",
      query_type_destructor,
      ERL_NIF_RT_CREATE,
      NULL);
  if (query_type == NULL)
  {
    return -1;
  }

  tree_type = enif_open_resource_type(
      env,
      NULL,
      "ts_tree",
      tree_type_destructor,
      ERL_NIF_RT_CREATE,
      NULL);
  if (tree_type == NULL)
  {
    return -1;
  }

  return 0;
}

static ErlNifFunc nif_funcs[] = {
  {"new", 0, ts_new},
  {"new_language", 2, ts_new_language},
  {"set_language", 2, ts_set_language},
  {"parse_string", 2, ts_parse_string},
  {"new_query", 2, ts_new_query},
  {"exec_query", 2, ts_exec_query}
};

ERL_NIF_INIT(Elixir.ExTreeSitter, nif_funcs, on_load, NULL, NULL, NULL)
