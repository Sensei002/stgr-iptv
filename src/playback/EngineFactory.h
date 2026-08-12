#pragma once

#include <QObject>

class QWidget;
class IPlaybackEngine;

// ---------------------------------------------------------------------------
// EngineFactory - creates the compiled-in playback backend.
// Returns nullptr when no backend is available (e.g. a build without libVLC);
// callers must handle that gracefully.
// ---------------------------------------------------------------------------
namespace EngineFactory {

IPlaybackEngine* create(QWidget* videoSurface, int networkCachingMs, int hwMode,
                        QObject* parent = nullptr);

} // namespace EngineFactory
