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
        /**
         * @brief Manages localization and translations for both editor and projects.
         *
         */
        class LocaleManager
        {
        public:
            /**
             * @brief Get the current locale
             *
             * @return const std::string& Current locale
             */
            static const std::string &GetCurrentLocale() noexcept;

            /**
             * @brief Set the user's preferred locale
             *
             * @param locale
             */
            static void SetUserLocale(std::string_view locale);

            /**
             * @brief Clear the user's preferred locale
             *
             */
            static void ClearUserLocale();

            /**
             * @brief Get the available locales
             *
             * @return const std::vector<std::string>& Available locales in the project
             */
            static const std::vector<std::string> &GetAvailableLocales() noexcept;

            /**
             * @brief Get the translation text for a given key
             *
             * @param key The key to translate
             * @return std::string_view The translated string, or the key itself if not found
             */
            static std::string_view GetTranslation(std::string_view key) noexcept;

            /**
             * @brief Exception thrown when a locale is not available
             *
             */
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
