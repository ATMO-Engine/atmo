#include <SDL3/SDL_locale.h>
#include <glaze/glaze.hpp>
#include <optional>

#include "glaze/core/common.hpp"
#include "locale_manager.hpp"
#include "project/file.hpp"
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

            if (m_user_preference.has_value() && hasLanguageAndCountryMatch(m_user_preference.value())) {
                m_current_locale = m_user_preference.value();
            } else {
                m_current_locale = detectSystemLocale();
            }

            loadTranslations();
        }

        const std::string &LocaleManager::GetCurrentLocale() noexcept
        {
            return Instance().m_current_locale;
        }

        void LocaleManager::SetUserLocale(std::string_view locale)
        {
            if (!Instance().hasLanguageAndCountryMatch(locale))
                throw LocaleNotAvailableException(locale.data());

            Instance().m_user_preference = locale;
            Instance().saveUserPreference();

            Instance().loadTranslations();
        }

        void LocaleManager::ClearUserLocale()
        {
            Instance().m_user_preference.reset();
            Instance().saveUserPreference();

            Instance().loadTranslations();
        }

        const std::vector<std::string> &LocaleManager::GetAvailableLocales() noexcept
        {
            return Instance().m_available_locales;
        }

        std::string_view LocaleManager::GetTranslation(std::string_view key) noexcept
        {
            auto &translations = Instance().m_translations;
            auto it = translations.find(std::string(key));
            if (it != translations.end()) [[likely]] {
                return it->second;
            } else {
                return key;
            }
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

                    if (hasLanguageAndCountryMatch(locale)) {
                        SDL_free(locales);
                        return locale;
                    }

                    if (auto best_match = getBestLanguageMatch(locales[i]->language); best_match.has_value()) {
                        SDL_free(locales);
                        return best_match.value();
                    }
                }
                SDL_free(locales);
            }

            for (const char *var : { "LANG", "LC_ALL", "LANGUAGE" }) {
                if (const char *value = std::getenv(var)) {
                    std::string locale(value);
                    locale = locale.substr(0, locale.find('.')).replace(locale.find('_'), 1, "-");
                    if (hasLanguageAndCountryMatch(locale)) {
                        return locale;
                    }
                    if (auto best_match = getBestLanguageMatch(locale.substr(0, 2)); best_match.has_value()) {
                        return best_match.value();
                    }
                }
            }

            return "en-US";
        }

        bool LocaleManager::hasLanguageAndCountryMatch(std::string_view language_and_country) const noexcept
        {
            return std::find(m_available_locales.begin(), m_available_locales.end(), language_and_country) != m_available_locales.end();
        }

        std::optional<std::string> LocaleManager::getBestLanguageMatch(std::string_view language) const noexcept
        {
            for (const auto &loc : m_available_locales)
                if (loc.substr(0, 2) == language)
                    return loc;

            return std::nullopt;
        }

        void LocaleManager::loadUserPreference() {}

        void LocaleManager::saveUserPreference() {}

        std::unordered_map<std::string, std::string> LocaleManager::subTranslationLoad(glz::generic::object_t *data, std::string_view root_key)
        {
            std::unordered_map<std::string, std::string> map;

            for (auto &[key, value] : *data) {
                auto full_key = std::format("{}.{}", root_key, key);
                if (auto obj = value.get_if<glz::generic::object_t>(); obj)
                    m_translations.merge(subTranslationLoad(obj, full_key));
                else if (auto str = value.get_if<std::string>(); str)
                    m_translations[full_key] = str->c_str();
                else
                    spdlog::error(R"(Unexpected value in translation file for key "{}")", key);
            }

            return map;
        }

        void LocaleManager::loadTranslations()
        {
            std::string_view locale = m_user_preference.has_value() ? m_user_preference.value() : m_current_locale;

            if (!hasLanguageAndCountryMatch(locale))
                throw LocaleNotAvailableException(locale.data());

            project::File file = project::FileSystem::OpenFile(std::format("project://translation/{}/atmo.json", locale));

            std::string content = file.readAll();
            glz::generic data{};
            auto err = glz::read_json(data, content);

            if (err) {
                std::string descriptive_error = glz::format_error(err, content);
                spdlog::error("Failed to load translations for '{}': {}", locale, descriptive_error);
            }

            m_translations.clear();

            for (auto &[key, value] : data.get_object()) {
                if (auto obj = value.get_if<glz::generic::object_t>(); obj)
                    m_translations.merge(subTranslationLoad(obj, key));
                else if (auto str = value.get_if<std::string>(); str)
                    m_translations[key] = str->c_str();
                else
                    spdlog::error(R"(Unexpected value in translation file for key "{}")", key);
            }
        }
    } // namespace locale
} // namespace atmo
