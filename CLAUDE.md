# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MaiMemoAiChat is a Qt (C++ + QML) desktop application for English learning. It combines spaced repetition (from 默默背单词/Anki engines) with AI-powered learning scenarios for listening, speaking, reading, and writing practice.

**Status**: Design phase — no source code yet. This repo only contains documentation (as a git submodule). Source code will be implemented under `src/`.

## Architecture (5 Modules)

```
UI ──> AppCoordinator ──> SceneOrchestrator ──> AI Service
                     │──> DataSync ──> SpacedRepetitionEngine
                     └──> Persistence (SQLite, settings, context)
```

| Module | Responsibility |
|--------|---------------|
| UI | QML interface — chat view, session list, input area |
| AppCoordinator | Defines interaction interfaces, coordinates workflows across modules |
| Persistence | Centralized storage (SQLite for messages/sessions/memories), deduplication, async write |
| SceneOrchestrator | Abstracts AI calls; generates learn scenes (listening/speaking/reading/writing) |
| DataSync | Abstracts spaced repetition engine interface (默默背单词/Anki); syncs memory data bidirectionally |

## Tech Stack

- **Language**: C++17, QML (Qt 6.x)
- **Build**: qmake or CMake
- **AI**: Anthropic Claude API (streaming)
- **Storage**: SQLite
- **SRS Interface**: Custom adapter for 默默背单词 / Anki

## Coding Conventions

- **Class names**: `PascalCase`
- **Methods**: `camelCase`
- **Member variables**: `m_camelCase`
- **Constants**: `UPPER_SNAKE_CASE`
- **QML component files**: `PascalCase.qml`
- **QML signals**: `camelCase`, verb-first
- **Commits**: Conventional Commits (`feat:`, `fix:`, `docs:`, `refactor:`, `test:`, `chore:`)

## Source Directory Layout (planned)

```
src/
├── core/           # Business logic (no UI dependency)
├── models/         # Data models
├── services/       # External service integrations (Claude API, SRS engine)
├── ui/             # QML files
└── utils/          # Utility classes
```

## Key Design Decisions

- **Centralized persistence**: All storage goes through one module for unified backup/sync
- **SRS adapter pattern**: `DataSync` abstracts the spaced repetition engine so the app doesn't depend on a specific provider
- **AI scene orchestration**: `SceneOrchestrator` abstracts AI calls, generating learning scenarios from user memory data
- **Memory injection**: Relevant memories are injected into AI context at conversation start; dynamic retrieval on explicit mention
- **No hardcoded API keys**: All credentials via environment or config file

## Docs

Documentation is maintained as a [separate repo](https://github.com/weiiiyan/MaiMemoAiChatDoc) mounted as a git submodule under `docs/`. Covers requirements, architecture, detailed design, testing, deployment, and project management.
