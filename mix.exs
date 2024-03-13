defmodule ExTreeSitter.MixProject do
  use Mix.Project

  def project do
    [
      app: :ex_tree_sitter,
      version: "0.0.1",
      elixir: "~> 1.15",
      start_permanent: Mix.env() == :prod,
      compilers: [:elixir_make] ++ Mix.compilers(),
      make_clean: ["clean"],
      description: "Elixir bindings for tree-sitter",
      package: package(),
      source_url: "https://github.com/puretype/ex_tree_sitter",
      deps: deps()
    ]
  end

  def application do
    [
      extra_applications: [:logger]
    ]
  end

  defp package do
    [
      files: ~w(lib mix.exs README* LICENSE* c_src nif Makefile),
      licenses: ["MIT"],
      links: %{"GitHub" => "https://github.com/puretype/ex_tree_sitter"}
    ]
  end

  defp deps do
    [
      {:ex_doc, ">= 0.0.0", only: :dev, runtime: false},
      {:mix_test_watch, "~> 1.0", only: [:dev, :test], runtime: false},
      {:elixir_make, "~> 0.4", runtime: false}
    ]
  end
end
