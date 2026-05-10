# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MaiMemoAiChat is a Qt (C++ + QML) desktop application for English learning. It combines spaced repetition (from 默默背单词/Anki engines) with AI-powered learning scenarios for listening, speaking, reading, and writing practice.

**Status**: Early implementation — source code lives under `MaiMemoAiChat/`. Documentation is in `docs/` (git submodule).

## Build

```bash
cd MaiMemoAiChat
cmake -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=<path-to-qt6>
cmake --build build
```

Requires: Qt 6.5+ (Core, Widgets), CMake 3.19+, C++17.

## Architecture (5 Modules)

```
UI ──> AppCoordinator ──> SceneOrchestrator ──> AI Service
                     │──> DataSync ──> SpacedRepetitionEngine
                     └──> Hold (Persistence)
```

| Module | Interface | Responsibility |
| ------ | --------- | -------------- |
| UI | IUIModule | QML interface — chat view, session list, input area |
| AppCoordinator | IAppCoordinator | Defines interaction interfaces, coordinates workflows across modules |
| Hold | Hold | Centralized file-based storage, async write with debounce — see interface at `docs/02-系统设计/2.2-接口设计/持久化模块接口.md` |
| SceneOrchestrator | ISceneOrchestrator | Abstracts AI calls; manages interactive learning sessions (reading/writing/listening/speaking) |
| DataSync | IDataSync | Abstracts SRS engine interface (默默背单词/Anki); syncs memory data bidirectionally via MemEntry |

Key architectural constraints:

- **All storage goes through Hold** — no module touches the filesystem directly.
- **Hold uses async writes** — writes are dispatched to a worker thread. Main thread reads from a pending-write cache or the filesystem. Writes are debounced (5s delay, reset on new write to same key).
- **Hold stores binary blobs** — serialization is the caller's responsibility. Keys are `(namespace: List<String>, name: String)` pairs, where namespace is a path like `["sessions", "session-abc"]`.

## Source Layout

```
MaiMemoAiChat/
├── CMakeLists.txt       # Top-level build
├── main.cpp / mainwindow.*  # Qt app entry point (scaffold)
├── Hold/                # Persistence module (in progress)
│   ├── Hold.h / Hold.cpp
│   ├── HoldWorker.h / HoldWorker.cpp
│   └── CMakeLists.txt
└── (other modules TBD)
```

Modules map to the planned `src/` structure from docs: `core/` (Hold, AppCoordinator), `services/` (SceneOrchestrator, DataSync), `ui/` (QML), `models/` (MemEntry, etc.), `utils/`.

## Tech Stack

- **Language**: C++17, QML (Qt 6.x)
- **Build**: CMake
- **AI**: Anthropic Claude API (streaming)
- **Storage**: File-based (binary, one file per key under namespace directories)
- **SRS Interface**: Custom adapter for 默默背单词 / Anki

## Coding Conventions

- **Class names**: `PascalCase` — interfaces prefixed with `I` (e.g., `IDataSync`), except `Hold` which follows the PlantUML spec
- **Methods**: `camelCase`
- **Member variables**: `m_camelCase`
- **Constants**: `UPPER_SNAKE_CASE`
- **QML component files**: `PascalCase.qml`
- **Commits**: Conventional Commits (`feat:`, `fix:`, `docs:`, `refactor:`, `test:`, `chore:`)
- **No hardcoded API keys** — all credentials via environment or config file
- **Comments**: Doxygen/JavaDoc 风格 (`/** ... */` / `@brief @param @return`)，关键逻辑和非显而易见的设计决策用中文注释

## Docs

Documentation is maintained as a [separate repo](https://github.com/weiiiyan/MaiMemoAiChatDoc) mounted as a git submodule under `docs/`. Key docs:

| Doc | Content |
| --- | ------- |
| `docs/02-系统设计/2.1-架构设计/2.1-架构设计.md` | Module responsibilities |
| `docs/02-系统设计/2.2-接口设计/持久化模块接口.md` | Hold interface spec |
| `docs/02-系统设计/2.2-接口设计/数据同步模块接口设计.md` | IDataSync + MemEntry spec |
| `docs/02-系统设计/2.2-接口设计/学习场景编排模块接口设计.md` | ISceneOrchestrator + SceneSession spec |
| `docs/02-系统设计/2.2-接口设计/应用协调模块接口设计.md` | IAppCoordinator spec |
