#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace atmo
{
    namespace locale
    {
        class LocaleManager
        {
        public:
            static const std::string &GetCurrentLocale() noexcept;
            static void SetUserLocale(std::string_view locale);
            static void ClearUserLocale();
            static const std::vector<std::string> &GetAvailableLocales() noexcept;

        private:
            std::string detectSystemLocale();
            bool hasTranslation(std::string_view locale) const noexcept;
            void loadUserPreference();
            void saveUserPreference();

            LocaleManager();
            ~LocaleManager() = default;

            static LocaleManager &Instance()
            {
                static LocaleManager instance;
                return instance;
            }

            std::optional<std::string> m_user_preference;
            std::string m_current_locale;
            std::vector<std::string> m_available_locales;
        };
    } // namespace locale
} // namespace atmo
