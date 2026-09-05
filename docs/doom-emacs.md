# Doom Emacs setup

Your Doom configuration already enables Corfu, `lsp-mode`, and C/C++ syntax.
The mlxgym setup adds a small `metal-mode` derived from `c++-mode` and registers
the experimental `metal-analyzer` language server specifically for `.metal`
buffers. This keeps clangd from claiming Metal files.

The resulting buffers have C++-style highlighting, Metal built-in completion,
hover help, compiler-backed diagnostics, and the existing two-space indentation.

## Objective-C++ host files

The project-level `.clangd` file points clangd at CMake's
`build/debug/compile_commands.json`. Generate it once after cloning:

```sh
cmake --preset debug
```

If `solution.mm` was already open, run `M-x lsp-workspace-restart` so clangd
reloads the compilation database. This supplies the framework include path,
Objective-C++ mode, ARC setting, and Apple framework flags used by the real
build.

The local-leader bindings are:

- `m t`: build and test the task containing the current buffer.
- `m b`: build and benchmark the current task.
- `m p`: capture a task GPU trace for Xcode.

`metal-analyzer` is third-party and experimental. Compiler diagnostics from
`xcrun metal` remain authoritative if the language server behaves differently.

After changing Doom configuration, run `doom sync` and restart Emacs.
