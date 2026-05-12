#include "macos_menu_bar.hpp"

#if !defined(ATMO_EXPORT)

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#include <functional>
#include <string>

#include "SDL3/SDL_keycode.h"
#include "locale/locale_manager.hpp"

@interface AtmoMenuItemTarget : NSObject
- (instancetype)initWithAction:(std::function<void()>)action;
- (void)menuItemClicked:(id)sender;
@end

@implementation AtmoMenuItemTarget {
    std::function<void()> _action;
}

- (instancetype)initWithAction:(std::function<void()>)action
{
    self = [super init];
    if (self) {
        _action = std::move(action);
    }
    return self;
}

- (void)menuItemClicked:(id)sender
{
    if (_action) {
        _action();
    }
}

@end

namespace atmo::editor
{
    struct MacOSMenuBar::Impl {
        NSMutableArray<AtmoMenuItemTarget *> *targets;

        Impl()
        {
            targets = [[NSMutableArray alloc] init];
        }
    };

    static NSString *sdlKeycodeToKeyEquivalent(SDL_Keycode key)
    {
        if (key >= SDLK_A && key <= SDLK_Z) {
            char c = static_cast<char>('a' + (key - SDLK_A));
            return [NSString stringWithFormat:@"%c", c];
        }
        if (key >= SDLK_0 && key <= SDLK_9) {
            char c = static_cast<char>('0' + (key - SDLK_0));
            return [NSString stringWithFormat:@"%c", c];
        }
        return @"";
    }

    static NSEventModifierFlags sdlModToNSMod(SDL_Keymod mod)
    {
        NSEventModifierFlags flags = 0;
        if (mod & SDL_KMOD_GUI) flags |= NSEventModifierFlagCommand;
        if (mod & SDL_KMOD_SHIFT) flags |= NSEventModifierFlagShift;
        if (mod & SDL_KMOD_ALT) flags |= NSEventModifierFlagOption;
        if (mod & SDL_KMOD_CTRL) flags |= NSEventModifierFlagControl;
        return flags;
    }

    MacOSMenuBar::MacOSMenuBar() : m_impl(new Impl()) {}

    MacOSMenuBar::~MacOSMenuBar()
    {
        delete m_impl;
    }

    void MacOSMenuBar::build(core::ecs::entities::Entity /*window_entity*/, Commands &commands)
    {
        NSMenu *mainMenu = [[NSMenu alloc] initWithTitle:@"MainMenu"];

        NSMenuItem *appItem = [[NSMenuItem alloc] initWithTitle:@"Atmo" action:nil keyEquivalent:@""];
        NSMenu *appMenu = [[NSMenu alloc] initWithTitle:@"Atmo"];

        NSMenuItem *aboutItem = [[NSMenuItem alloc] initWithTitle:@"About Atmo" action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
        [aboutItem setTarget:NSApp];
        [appMenu addItem:aboutItem];
        [appMenu addItem:[NSMenuItem separatorItem]];

        AtmoMenuItemTarget *quitTarget = [[AtmoMenuItemTarget alloc] initWithAction:[&commands]() { commands.execute("file.quit"); }];
        [m_impl->targets addObject:quitTarget];

        NSMenuItem *quitItem = [[NSMenuItem alloc] initWithTitle:@"Quit Atmo"
                                                          action:@selector(menuItemClicked:)
                                                   keyEquivalent:@"q"];
        [quitItem setTarget:quitTarget];
        [quitItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
        [appMenu addItem:quitItem];

        [appItem setSubmenu:appMenu];
        [mainMenu addItem:appItem];

        for (auto &[category, cmds] : commands.byCategory()) {
            auto translated_category = atmo::locale::LocaleManager::GetTranslation(category);
            NSString *catNS = [NSString stringWithUTF8String:std::string(translated_category).c_str()];

            NSMenuItem *catItem = [[NSMenuItem alloc] initWithTitle:catNS action:nil keyEquivalent:@""];
            NSMenu *catMenu = [[NSMenu alloc] initWithTitle:catNS];

            for (const Command *cmd : cmds) {
                if (!cmd) {
                    [catMenu addItem:[NSMenuItem separatorItem]];
                    continue;
                }

                auto translated_label = atmo::locale::LocaleManager::GetTranslation(cmd->id);
                NSString *label = [NSString stringWithUTF8String:std::string(translated_label).c_str()];

                NSString *keyEquiv = @"";
                NSEventModifierFlags modFlags = 0;

                if (cmd->shortcut.has_value()) {
                    keyEquiv = sdlKeycodeToKeyEquivalent(cmd->shortcut->key);
                    modFlags = sdlModToNSMod(cmd->shortcut->modifiers);
                }

                AtmoMenuItemTarget *target = [[AtmoMenuItemTarget alloc] initWithAction:[id = cmd->id, &commands]() {
                    commands.execute(id);
                }];
                [m_impl->targets addObject:target];

                NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:label
                                                              action:@selector(menuItemClicked:)
                                                       keyEquivalent:keyEquiv];
                [item setTarget:target];
                [item setKeyEquivalentModifierMask:modFlags];

                [catMenu addItem:item];
            }

            [catItem setSubmenu:catMenu];
            [mainMenu addItem:catItem];
        }

        [NSApp setMainMenu:mainMenu];
    }

    std::unique_ptr<IPlatformMenuBar> makePlatformMenuBar()
    {
        return std::make_unique<MacOSMenuBar>();
    }
} // namespace atmo::editor

#endif
