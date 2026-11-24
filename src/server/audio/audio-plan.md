# Plan d'implémentation — Audio

## Principe

- `AudioServer` (`atmo::server::audio`) : ne connaît **ni flecs ni les entités**. Il possède les samples et les voix,
  mixe et envoie le son à SDL. API en RID uniquement.
- `AudioSample` (`core/resource`) : handle RAII sur un `SampleRid`, chargé et dédupliqué par le `ResourceManager`.
- `AudioEmitter` (entité + composant) : path + réglages. Le système `AudioEmitter_Sync` détecte les changements et les
  **pousse** vers le server. Même modèle que `Sprite2d` (`src/core/ecs/entities/2d/sprite_2d/sprite_2d.cpp`).

```
AudioEmitter ──(AudioEmitter_Sync, thread principal)──▶ AudioServer (mutex) ◀── Callback SDL (thread audio) → mix()
     │                                                       ▲
     └── ResourceManager::getResource<AudioSample>(path) ────┘ (loader → sampleCreate)
```

- Format interne unique : **float32, stéréo entrelacée, 48 kHz**. Toute conversion se fait dans le loader.
- **Sample** = les données d'un son, partagées. **Voice** = une lecture en cours. Un emitter possède une voix.

## Déjà fait

- [x] RID : `SampleRid`, `VoiceRid` et `RidAllocator` (`rid.hpp`, `rid_allocator.hpp`).
- [x] `AudioServer` : `Instance`, `init`, `shutdown`, `isInitialized`, callback SDL.
- [x] Samples : `sampleCreate`, `sampleFree`, `sampleGetFrames`.
- [x] Voix : `voiceCreate`, `voiceFree`, `voiceSetSample`, `voicePlay`, `voiceStop`, `voiceSetPaused`, `voiceSetVolume`,
      `voiceSetPitch`, `voiceSetPan`, `voiceSetLoop`, `voiceIsPlaying`.
- [x] `mix()` : volume, pan, pitch avec interpolation linéaire, boucle, fin de lecture, clamp [-1, 1].

---

# 1. À finir

Objectif : un server robuste, branché au moteur, dont on entend un son.

### 1.1 Débloquer le build

- [x] Neutraliser `src/core/ecs/entities/audio/audio_emitter/audio_emitter.cpp` : il contient encore du code de
      `Sprite2d` et ne compile pas, or `xmake.lua` prend tous les `.cpp` de `src/`.

### 1.2 Corriger `audio_server.cpp`

- [x] Refuser les `NaN` et infinis (`std::isfinite`) dans `voiceSetVolume`, `voiceSetPitch` et `voiceSetPan`.
      Ils traversent `std::max` / `std::clamp` ; un pitch `NaN` rend le curseur `NaN` et `mix()` lit hors du tableau.
- [x] Vérifier le retour de `SDL_ResumeAudioStreamDevice` dans `init()`. En cas d'échec : log, `SDL_DestroyAudioStream`,
      `m_stream = nullptr`, `return false`.
- [x] Ajouter `#include <utility>` (`std::move`).
- [x] Corriger le commentaire de `~AudioServer()` : si le premier `Instance()` vient après `initSDL()`, le destructeur
      s'exécute **avant** `SDL_Quit`. La garde `SDL_WasInit` couvre l'ordre inverse.

### 1.3 Brancher le server au moteur

- [x] `src/core/engine.cpp`, `Engine::init()` : appeler `AudioServer::Instance().init()` après l'init du
      `RenderingServer`. Simple `spdlog::warn` en cas d'échec : un jeu sans son reste jouable.
- [x] Ne pas ouvrir la sortie si `m_headless`. Samples et voix restent utilisables sans device.
- [x] `src/core/engine.hpp` : `AudioServerShutdown` RAII, sur le modèle de `RenderingServerShutdown`.
- Rien à ajouter dans `xmake.lua`.

### 1.4 Écouter un vrai son

- [x] Test manuel temporaire : sinusoïde 440 Hz d'une seconde → `sampleCreate` → `voiceCreate` → `voiceSetSample` →
      `voicePlay`. Vérifier le bip, le pitch ×2, le pan, la boucle et la fermeture propre du moteur. Retirer ensuite.

### 1.5 Tests automatiques

`tests/server/audio/`, sans carte son, en appelant `mix()` directement :

- [x] RID : un RID libéré n'est plus valide, même si son slot est réutilisé.
- [x] `sampleCreate` refuse un nombre impair de floats.
- [x] Lecture normale, fin de lecture, boucle, pause, stop.
- [x] Volume, pitch, pan et interpolation.
- [x] Somme de plusieurs voix et clamp.
- [x] `sampleFree` coupe les voix qui utilisaient ce sample.
- [x] Valeurs `NaN` et infinies ignorées.

**Terminé quand** : le projet compile, les tests passent et le bip s'entend.

---

# 2. Pour le jeu

Objectif : placer un `AudioEmitter` dans une scène et jouer un `.wav`.

j'ai supprimer l'audio emitter pour te laisser le redisgn ou pas, par rapport au besoin de la connexion au moteur, tu peux repartir sur la meme base, le but dans notre
logique etait d'avoir des entité audio emiteeur qui seront enfant d'entité, donc AudioEmitter, AudioEmitter2d avec des position pour des futur calcul dans le serveur par
rapport a la dispertion de son ..., AudioEmitter meme attaché a un player par exemple ne fait qu'envoyer du son simple, ca sera surtout utiliser pour de la musique ambiante
ou des sond effect simple de menuing par exemple, a présent je te laisse

### 2.1 Ressource `AudioSample`

`src/core/resource/audio_sample.hpp/.cpp`, calquée sur `texture_2d.hpp/.cpp`.

- [x] Constructeur `AudioSample(SampleRid)`, `rid()`, `duration()` (`sampleGetFrames / sample_rate`), non copiable.
- [x] Destructeur → `AudioServer::Instance().sampleFree(m_rid)`.

### 2.2 Loader `.wav`

`src/core/resource/loaders/audio_sample_loader.hpp/.cpp`, calqué sur `texture_2d_loader`.

- [x] `project::FileSystem::OpenFile(path)` → `SDL_LoadWAV_IO(file.toIOStream(), true, ...)`.
- [x] `SDL_ConvertAudioSamples` vers float32 stéréo 48 kHz → `sampleCreate`.
- [x] Libérer les deux buffers SDL avec `SDL_free`, y compris en cas d'erreur.
- [x] `LoadException` si le décodage, la conversion ou la création échoue. Ne pas exiger `isInitialized()` : le
      chargement doit marcher en headless.
- [x] `ATMO_REGISTER_RESOURCE_LOADER(..., .display_name = "Audio", .extensions = { ".wav" })`.

### 2.3 Composant `AudioEmitter`

`src/core/ecs/entities/audio/audio_emitter/`

```cpp
struct AudioEmitter {
    std::string audio_path;
    float volume = 1.0f;
    float pitch = 1.0f;
    float pan = 0.0f;
    bool loop = false;
    bool autoplay = false;

    // runtime, non exposé
    std::string prev_audio_path;
    std::shared_ptr<resource::AudioSample> m_sample;
    server::audio::VoiceRid m_voice;
    float m_volume = 1.0f; // dernières valeurs envoyées au server
    float m_pitch = 1.0f;
    float m_pan = 0.0f;
    bool m_loop = false;
    bool m_autoplay_done = false;
};
```

- [ ] `ComponentMeta` : exposer `audio_path` (widget `file_path`), `volume`, `pitch`, `pan`, `loop`, `autoplay`.
- L'état de lecture reste dans le server (`voiceIsPlaying`), pas de booléen `playing` à synchroniser.

### 2.4 Entité et systèmes

- [ ] Réécrire `audio_emitter.cpp` : includes, `initialize()`, `RegisterSystems()`, `LoadAudio()`.
- [ ] `AudioEmitter_Sync` (`PreUpdate`) :
  - crée la voix si elle est invalide ;
  - si `audio_path` a changé : recharge via le `ResourceManager`, puis `voiceSetSample` ;
  - envoie `volume`, `pitch`, `pan` et `loop` seulement s'ils ont changé ;
  - lance `autoplay` une seule fois, en jeu uniquement.
- [ ] Trouver comment distinguer l'éditeur du jeu pour `autoplay` (regarder `WorldContext::is_editor_isolated`).
- [ ] Observer `OnRemove` : `voiceFree`, puis `m_sample = nullptr`.
- [ ] Vérifier la copie d'entités : deux emitters ne doivent pas partager le même `m_voice`.
- [ ] Méthodes : `setAudioPath` / `getAudioPath`, `play`, `stop`, `setPaused`, `isPlaying`, accesseurs `volume`, `pitch`,
      `pan`, `loop`. `play()` appelle directement `voicePlay`, pour pouvoir relancer un son déjà en cours.
- [ ] `ATMO_REGISTER_ENTITY(entities::AudioEmitter)` et
      `ATMO_REGISTER_COMPONENT(atmo::core::components::AudioEmitter)`.

### 2.5 Scripts Luau

- [ ] Bindings dans `src/luau/bindings/entity/`, sur le modèle de `bindings_sprite_entity.hpp/.cpp`.

### 2.6 Tests

- [ ] Loader : un `.wav` mono 44,1 kHz donne un sample stéréo 48 kHz de la bonne durée ; un fichier invalide lance
      `LoadException`.
- [ ] `ResourceManager` : deux chargements du même path renvoient le même `shared_ptr`.
- [ ] ECS : un changement de réglage atteint la voix après un tick ; supprimer l'entité libère la voix.

**Terminé quand** : un `.wav` se joue depuis une scène et depuis un script Luau.

---

# 3. Plus tard

### 3.1 Bus

- [x] `enum class Bus { Master, Music, Sfx }` fixe pour commencer, pas de RID.
- [x] `voiceSetBus`, `busSetVolume` ; `mix()` additionne les voix par bus, puis les bus dans Master.
- [x] Champ `bus` dans `AudioEmitter`.
- [x] Luau : `Audio.setBusVolume` / `Audio.getBusVolume`. Le volume d'un bus est global, jamais porté par une entité.
- [ ] `BusRid` seulement si les bus deviennent configurables dans les settings du projet.

### 3.2 Effets DSP (`src/server/audio/effects/`)

```cpp
class AudioEffect {
public:
    virtual ~AudioEffect() = default;
    virtual void process(float *stereo, std::size_t frames) = 0;
};
```

- [x] Sur les bus : `LowPass`, `HighPass`, `Eq3Band`, et `Limiter` sur Master à la place du clamp.
- [x] Ensuite : `Reverb`, `Delay`, ducking.
- [x] Luau : `Audio.setEffect` / `getEffect` / `removeEffect`, `Audio.setDucking` / `removeDucking`.
- [ ] Effets par entité seulement en cas de besoin réel.

### 3.3 Son spatial 2D

- [ ] `Transform2d` + listener (caméra) → pan et atténuation automatiques.

### 3.4 Formats et streaming

- [ ] `.ogg`, `.mp3`, `.flac` (choisir une lib).
- [ ] Streaming des musiques longues, dans une ressource `AudioStream` distincte de `AudioSample`.

### 3.5 Qualité et performances

- [ ] Lissage de volume / pan et petit fondu au stop, pour éviter les clics.
- [ ] Pan à puissance constante.
- [ ] Limite du nombre de voix et priorités (voice stealing).
- [ ] File de commandes à la place du mutex si la contention devient un problème.
- [ ] Mutualiser `Rid` / `RidAllocator` avec le rendering (`src/server/common/`).

---

## Ordre conseillé

1.1 → 1.2 → 1.3 → 1.4 → 1.5 → 2.1 → 2.2 → 2.3 → 2.4 → 2.5 → 2.6, puis la partie 3 selon les besoins.
