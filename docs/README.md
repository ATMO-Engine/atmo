# Documentation pages

The guides and manual of [docs.atmo-engine.com](https://docs.atmo-engine.com). They live here so they change in the same pull requests as the code they describe, and each engine version publishes the pages it ships with.

| Path                  | Published at        |
| --------------------- | ------------------- |
| `en/getting-started/` | `/getting-started/` |
| `en/manual/`          | `/manual/`          |

The site, its home page and the C++ API reference tooling are in [ATMO-Engine/docs](https://github.com/ATMO-Engine/docs).

## Writing pages

Pages are Markdown with a frontmatter `id`, and link to each other and to C++ symbols with `[[references]]`:

```md
See [[building-from-source#build]] and [[RenderingServer]].
```

See [Writing documentation](https://docs.atmo-engine.com/latest/contributing/writing-documentation) for the frontmatter fields and the reference syntax. References render as plain text on GitHub; they become links on the site.

## Previewing

Clone [ATMO-Engine/docs](https://github.com/ATMO-Engine/docs) next to this repository, then from it:

```sh
pnpm install
pnpm api                              # C++ API model, needed by symbol references
pnpm dev                              # http://localhost:5173/latest/, reloads when these pages change
pnpm exec prettier --write ../atmo/docs
```

The `docs` workflow runs the same build on every pull request, and fails on unformatted pages or broken references.
