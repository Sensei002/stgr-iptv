#include "playback/EngineFactory.h"

#ifdef STGR_WITH_VLC
#include "playback/VlcPlaybackEngine.h"
#endif

namespace EngineFactory {

IPlaybackEngine* create(QWidget* videoSurface, int networkCachingMs, int hwMode,
                        QObject* parent)
{
#ifdef STGR_WITH_VLC
    auto* engine = new VlcPlaybackEngine(videoSurface, networkCachingMs, hwMode, parent);
    if (engine->isValid())
        return engine;
    delete engine;
    return nullptr;
#else
    Q_UNUSED(videoSurface);
    Q_UNUSED(networkCachingMs);
    Q_UNUSED(hwMode);
    Q_UNUSED(parent);
    return nullptr;
#endif
}

} // namespace EngineFactory
