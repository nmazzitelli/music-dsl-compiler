[![✗](https://img.shields.io/badge/Release-v2.0.0-ffb600.svg?style=for-the-badge)](https://github.com/nmazzitelli/music-dsl-compiler/releases)

[![✗](https://github.com/nmazzitelli/music-dsl-compiler/actions/workflows/pipeline.yaml/badge.svg?branch=development)](https://github.com/nmazzitelli/music-dsl-compiler/actions/workflows/pipeline.yaml)

# Music DSL Compiler

A compiler for a Music DSL built with Flex and Bison. It parses music programs and builds an AST.

* [Requirements](#requirements)
* [Quick Start](#quick-start)
* [Configuration](#configuration)
* [Commands](#commands)
* [CI/CD](#cicd)
* [Recommended Extensions](#recommended-extensions)

## Requirements

* [Docker v28.3.2](https://www.docker.com/)

## Quick Start

Make sure Docker Engine is running, then execute:

```bash
docker compose run --rm compiler
chmod +x src/main/bash/*.sh
bash src/main/bash/build.sh
bash src/main/bash/test.sh
```

## Configuration

Set the following environment variables to control and configure the behaviour of the application:

| Name                  | Default | Description                                                                                                                                                           |
| :-------------------- | :-----: | :-------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `ENVIRONMENT`         | `Local` | The active environment name. The available environments are: `Local`, `Development` and `Production`.                                                                 |
| `LOG_IGNORED_LEXEMES` | `true`  | When `true`, logs all of the ignored lexemes found with Flex at `DEBUGGING` level. To remove those logs from the console output set it to `false`.                    |
| `LOGGING_LEVEL`       | `ALL`   | The minimum level to log in the console output. From lower to higher, the available levels are: `ALL`, `DEBUGGING`, `INFORMATION`, `WARNING`, `ERROR` and `CRITICAL`. |

_Docker Compose_ can read the variables from an `.env` file too (see `compose.yaml` file).

## Commands

Before running commands, make sure:

* Docker Engine is running.
* Shell scripts have execution permissions in your local checkout (required in some Linux/WSL bind-mounted environments):

```bash
chmod +x src/main/bash/*.sh
```

### Start development container

Starts an ephemeral container ready for development:

```bash
docker compose run --rm compiler
```

### Build compiler

Builds or rebuilds the compiler and generates `.build/Flex-Bison-Compiler`:

```bash
bash src/main/bash/build.sh
```

### Run compiler

Compiles a program from a file:

```bash
bash src/main/bash/run.sh <program>
```

where `<program>` is the path to the file that represents its entry-point.

### Run tests

Runs all acceptance and rejection tests under `src/test/c`:

```bash
bash src/main/bash/test.sh
```

### Stop container

Exit the container and shut down the Compose services/network:

```bash
exit
docker compose down
```

### Docker

| Command                                 | Description                                             |
| :-------------------------------------- | :------------------------------------------------------ |
| `docker builder prune --all`            | Removes all builds and complete build cache.            |
| `docker compose --progress=plain build` | Forces a build or rebuild of the images in the cluster. |
| `docker image prune`                    | Removes all of the dangling images from Docker.         |
| `docker network prune`                  | Removes unused networks from Docker.                    |
| `docker volume prune`                   | Removes unused volumes from Docker.                     |

## CI/CD

To trigger an automatic integration on every push or PR (_Pull Request_), you must activate _GitHub Actions_ in the _Settings_ tab. Use the following configuration:

| Key                                                        | Value                                               |
| :--------------------------------------------------------- | :-------------------------------------------------- |
| `Actions permissions`                                      | `Allow all actions and reusable workflows`          |
| `Allow GitHub Actions to create and approve pull requests` | `false`                                             |
| `Artifact and log retention`                               | `30 days`                                           |
| `Fork pull request workflows from outside collaborators`   | `Require approval for all outside collaborators`    |
| `Workflow permissions`                                     | `Read repository contents and packages permissions` |

## Recommended Extensions

* [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
* [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)
* [Yash](https://marketplace.visualstudio.com/items?itemName=daohong-emilio.yash)
