#pragma once

#include <exception>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "glaze/glaze.hpp"

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
            static std::string_view GetTranslation(std::string_view key) noexcept;

            class LocaleNotAvailableException : public std::exception
            {
            public:
                LocaleNotAvailableException(const std::string &locale) : m_locale(locale) {}

                const char *what() const noexcept override
                {
                    return "The specified locale is not available.";
                }

                const std::string &getLocale() const noexcept
                {
                    return m_locale;
                }

            private:
                std::string m_locale;
            };

        private:
            std::string detectSystemLocale();
            bool hasLanguageAndCountryMatch(std::string_view language_and_country) const noexcept;
            std::optional<std::string> getBestLanguageMatch(std::string_view language) const noexcept;
            void loadUserPreference();
            void saveUserPreference();
            [[nodiscard]] std::unordered_map<std::string, std::string> subTranslationLoad(glz::generic::object_t *data, std::string_view key);
            void loadTranslations();

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
            std::unordered_map<std::string, std::string> m_translations;
        };
    } // namespace locale
} // namespace atmo
