defmodule ExTreeSitter do
  @moduledoc """
  NIF entrypoints for ExTreeSitter
  """

  @compile {:autoload, false}
  @on_load {:load_nif, 0}

  def load_nif() do
    :ok = :erlang.load_nif('./nif/tree_sitter_nif', 0)
  end

  def new(), do: :erlang.nif_error(:not_loaded)
  def new_language(_library, _function), do: :erlang.nif_error(:not_loaded)
  def set_language(_parser, _language), do: :erlang.nif_error(:not_loaded)
  def parse_string(_parser, _source_code), do: :erlang.nif_error(:not_loaded)
  def query(_language, _query, _tree), do: :erlang.nif_error(:not_loaded)
end
