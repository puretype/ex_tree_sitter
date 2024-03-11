defmodule ExTreeSitterTest do
  use ExUnit.Case

  test "returns a parser" do
    assert is_reference(ExTreeSitter.new())
  end
end
