defmodule ExTreeSitter do
  @moduledoc """
  NIF entrypoints for ExTreeSitter
  """

  @compile {:autoload, false}
  @external_resource "../nif/tree_sitter_nif.c"
  @on_load {:load_nif, 0}

  def load_nif() do
    path = Application.app_dir(:ex_tree_sitter, ["priv", "nif", "tree_sitter_nif"])
    :ok = :erlang.load_nif(path, 0)
  end

  def new(), do: :erlang.nif_error(:not_loaded)
  def new_language(_library, _function), do: :erlang.nif_error(:not_loaded)
  def set_language(_parser, _language), do: :erlang.nif_error(:not_loaded)
  def parse_string(_parser, _source_code), do: :erlang.nif_error(:not_loaded)

  def new_query(_language, _source), do: :erlang.nif_error(:not_loaded)
  def exec_query(_query, _tree), do: :erlang.nif_error(:not_loaded)
end
