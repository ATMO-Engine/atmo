#include <exception>
#include <fstream>
#include <memory>
#include <stdexcept>

#include "core/resource/loaders/script_loader.hpp"
#include "luau/luau.hpp"
#include "project/file_system.hpp"
#include "script_loader.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            ScriptLoader::ScriptLoader() {}

            ScriptLoader::~ScriptLoader() {}

            std::shared_ptr<Bytecode> ScriptLoader::load(const std::string &path)
            {
                auto file = project::FileSystem::OpenFile(path);

                Bytecode *newRessource = nullptr;
                try {
                    size_t bytecodeSize = 0;
                    char *bytecode = atmo::luau::Luau::Compile(file.readAll(), &bytecodeSize, path);

                    if (!bytecode) {
                        throw LoadException("Failed to load bytecode: " + path);
                    }

                    Bytecode *newRessource = new Bytecode{};
                    if (!newRessource) {
                        throw LoadException("Failed to load bytecode: " + path);
                    }
                    newRessource->data = bytecode;
                    newRessource->size = bytecodeSize;
                    return std::shared_ptr<Bytecode>(newRessource, [](Bytecode *b) {
                        if (b) {
                            if (b->data) {
                                free(b->data);
                            }
                            delete b;
                        }
                    });
                } catch (const LoadException &e) {
                    throw e;
                } catch (const std::exception &e) {
                    std::string expCatch = e.what();
                    throw LoadException("catched " + expCatch + "during script loading");
                }
            }

            const std::string ScriptLoader::resourceTypeName()
            {
                return "Script";
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
// namespace atmo
