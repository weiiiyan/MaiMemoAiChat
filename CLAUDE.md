# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Common guidelines

### 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

### 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

### 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

### 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

## Project Overview

MaiMemoAiChat is a Qt (C++ + QML) desktop application for English learning. It combines spaced repetition (from 墨墨背单词/Anki engines) with AI-powered learning scenarios for listening, speaking, reading, and writing practice.

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
| Hold | Hold | Centralized file-based storage, synchronous atomic writes — see interface at `docs/02-系统设计/2.2-接口设计/持久化模块接口.md` |
| SceneOrchestrator | ISceneOrchestrator | Abstracts AI calls; manages interactive learning sessions (reading/writing/listening/speaking) |
| DataSync | IDataSync | Abstracts SRS engine interface (墨墨背单词/Anki); syncs memory data bidirectionally via MemEntry |

Key architectural constraints:

- **All storage goes through Hold** — no module touches the filesystem directly.
- **Hold uses synchronous writes** — all I/O is direct, using QSaveFile for atomic writes.
- **Hold stores binary blobs** — serialization is the caller's responsibility. Keys are `(namespace: List<String>, name: String)` pairs, where namespace is a path like `["sessions", "session-abc"]`.

## Source Layout

```
MaiMemoAiChat/
├── CMakeLists.txt       # Top-level build
├── main.cpp / mainwindow.*  # Qt app entry point (scaffold)
├── Hold/                # Persistence module (in progress)
│   ├── Hold.h / Hold.cpp
│   └── CMakeLists.txt
└── (other modules TBD)
```

Modules map to the planned `src/` structure from docs: `core/` (Hold, AppCoordinator), `services/` (SceneOrchestrator, DataSync), `ui/` (QML), `models/` (MemEntry, etc.), `utils/`.

## Tech Stack

- **Language**: C++17, QML (Qt 6.x)
- **Build**: CMake
- **AI**: Anthropic Claude API (streaming)
- **Storage**: File-based (binary, one file per key under namespace directories)
- **SRS Interface**: Custom adapter for 墨墨背单词 / Anki

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
