#pragma once

#include <cmath>

namespace wvwfightanalysis::gui {

    enum class ContentState {
        Loading,
        Empty,
        Ready
    };

    inline ContentState ResolveContentState(bool hasData, bool loadingComplete) {
        if (hasData)
            return ContentState::Ready;

        return loadingComplete ? ContentState::Empty : ContentState::Loading;
    }

    class ContentTransition {
    public:
        void Update(ContentState nextState, float deltaTime, float fadeDuration = 0.32f) {
            if (nextState != m_state) {
                m_state = nextState;
                m_readyAlpha = nextState == ContentState::Ready ? 0.0f : 1.0f;
            }

            if (m_state != ContentState::Ready)
                return;

            if (fadeDuration <= 0.0f) {
                m_readyAlpha = 1.0f;
                return;
            }

            const float blend = 1.0f - std::exp(-deltaTime / fadeDuration);
            m_readyAlpha += (1.0f - m_readyAlpha) * blend;
            if (m_readyAlpha > 0.999f)
                m_readyAlpha = 1.0f;
        }

        ContentState State() const {
            return m_state;
        }

        bool HasContent() const {
            return m_state == ContentState::Ready;
        }

        float ContentAlpha() const {
            return m_state == ContentState::Ready ? m_readyAlpha : 0.0f;
        }

        float PlaceholderAlpha() const {
            return m_state == ContentState::Ready ? 1.0f - m_readyAlpha : 1.0f;
        }

    private:
        ContentState m_state = ContentState::Loading;
        float m_readyAlpha = 0.0f;
    };

} // namespace wvwfightanalysis::gui
