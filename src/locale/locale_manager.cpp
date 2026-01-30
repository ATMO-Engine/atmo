#include <SDL3/SDL_locale.h>

#include "locale_manager.hpp"
#include "project/file_system.hpp"

namespace atmo
{
    namespace locale
    {
        LocaleManager::LocaleManager()
        {
#if !defined(ATMO_EXPORT)
            std::vector<std::string> locales = atmo::project::FileSystem::SearchFiles("project://translation/*/atmo.json");
            for (const auto &path : locales) {
                std::string locale = path.substr(path.find("translation/") + 12);
                locale = locale.substr(0, locale.find("/"));
                m_available_locales.push_back(locale);
            }
#endif

            loadUserPreference();

            if (m_user_preference.has_value() && hasTranslation(m_user_preference.value())) {
                m_current_locale = m_user_preference.value();
            } else {
                m_current_locale = detectSystemLocale();
            }
        }

        const std::string &LocaleManager::GetCurrentLocale() noexcept
        {
            return Instance().m_current_locale;
        }

        void LocaleManager::SetUserLocale(std::string_view locale) {}

        void LocaleManager::ClearUserLocale() {}

        const std::vector<std::string> &LocaleManager::GetAvailableLocales() noexcept
        {
            return Instance().m_available_locales;
        }


        std::string LocaleManager::detectSystemLocale()
        {
            int count = 0;
            SDL_Locale **locales = SDL_GetPreferredLocales(&count);

            if (locales != nullptr && count > 0) {
                for (int i = 0; i < count; ++i) {
                    std::string locale = locales[i]->language;
                    if (locales[i]->country) {
                        locale += "-" + std::string(locales[i]->country);
                    }

                    if (hasTranslation(locale)) {
                        SDL_free(locales);
                        return locale;
                    }
                }
                SDL_free(locales);
            }

            for (const char *var : { "LANG", "LC_ALL", "LANGUAGE" }) {
                if (const char *value = std::getenv(var)) {
                    std::string locale(value);
                    locale = locale.substr(0, locale.find('.'));
                    if (hasTranslation(locale)) {
                        return locale;
                    }
                }
            }

            return "en-US";
        }

        bool LocaleManager::hasTranslation(std::string_view locale) const noexcept
        {
            if (std::find(m_available_locales.begin(), m_available_locales.end(), locale) != m_available_locales.end())
                return true;

            std::string lang_code = std::string(locale).substr(0, 2);
            for (const auto &loc : m_available_locales)
                if (loc.substr(0, 2) == lang_code)
                    return true;

            return false;
        }

        void LocaleManager::loadUserPreference() {}

        void LocaleManager::saveUserPreference() {}
    } // namespace locale
} // namespace atmo
