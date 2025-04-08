# GodotSteam MultiplayerPeer for Godot Engine 4.x | Community Edition
An ecosystem of tools for [Godot Engine](https://godotengine.org) and [Valve's Steam](https://store.steampowered.com). MultiplayerPeer bridges Godot's multiplayer nodes with Steam networking. For the Windows, Linux, and Mac platforms.

Additional Flavors
---
Pre-Compiles | Plug-ins | Server | Examples
--- | --- | --- | ---
[Godot 2.x](https://github.com/GodotSteam/GodotSteam/tree/godot2) | [GDNative](https://github.com/GodotSteam/GodotSteam/tree/gdnative) | [Server 3.x](https://github.com/GodotSteam/GodotSteam-Server/tree/godot3) | [Skillet](https://github.com/GodotSteam/Skillet)
[Godot 3.x](https://github.com/GodotSteam/GodotSteam/tree/godot3) | [GDExtension](https://github.com/GodotSteam/GodotSteam/tree/gdextension) | [Server 4.x](https://github.com/GodotSteam/GodotSteam-Server/tree/godot4) | ---
[Godot 4.x](https://github.com/GodotSteam/GodotSteam/tree/godot4) | --- | [GDNative](https://github.com/GodotSteam/GodotSteam-Server/tree/gdnative) | ---
[MultiplayerPeer](https://github.com/GodotSteam/MultiplayerPeer)| --- | [GDExtension](https://github.com/GodotSteam/GodotSteam-Server/tree/gdextension) | ---

Documentation
---
[Documentation is available here](https://godotsteam.com/). You can also check out the Search Help section inside Godot Engine.

Feel free to chat with us or ask questions on the [GodotSteam Discord server](https://discord.gg/SJRSq6K).

Donate
---
Pull-requests are the best way to help the project out but you can also donate through [Github Sponsors](https://github.com/sponsors/Gramps)!

Current Build
---
You can [download pre-compiled versions of this repo here](https://github.com/GodotSteam/MultiplayerPeer/releases).

**Version 4.14 Changes**
- Changed: version bump to match Godot 4.x branch update

[You can read more change-logs here](https://godotsteam.com/changelog/multiplayer_peer/).

Compatibility
---
While rare, sometimes Steamworks SDK updates will break compatilibity with older GodotSteam versions. Any compatability breaks are noted below. Newer API files (dll, so, dylib) _should_ still work for older versions.

Steamworks SDK Version | GodotSteam Version
---|---
1.62 or newer | 4.14 or newer
1.61 | 4.12 to 4.13
1.60 | 4.6 to 4.11
1.59 | 4.6 to 4.8
1.58a or older | 4.5.4 or older

Versions of GodotSteam that have compatibility breaks introduced.

GodotSteam Version | Broken Compatibility
---|---
4.8 | Networking identity system removed, replaced with Steam IDs
4.9 | sendMessages returns an Array
4.11 | setLeaderboardDetailsMax removed
4.13 | getItemDefinitionProperty return a dictionary, html_needs_paint key 'bgra' changed to 'rbga'
4.14 | Removed first argument for stat request in steamInit and steamInitEx, steamInit returns intended bool value

Known Issues
---
- Steam overlay may not work when running your game from the editor if you are using Forward+ as the renderer unless you use auto-initialization from the Project Settings menu.  Your exported project should work perfectly fine in the Steam client, however.
- When self-compiling, **do not** use MinGW as it will cause crashes.

Quick How-To
---
For complete instructions on how to build the GodotSteam MultiplayerPeer from scratch, [please refer to our documentation's 'How-To Multiplayer Peer' section.](https://godotsteam.com/howto/multiplayer_peer/) It will have the most up-to-date information.

Alternatively, you can just [download the pre-compiled versions in our Releases section](https://github.com/GodotSteam/MultiplayerPeer/releases) and skip compiling it yourself!

[To start, check out our tutorial on initializing Steam.](https://godotsteam.com/tutorials/initializing/)  There are additional tutorials with more in the works.  You can also [check out additional Godot and Steam related videos, text, additional tools, plug-ins, etc. here.](https://godotsteam.com/tutorials/external/)

License
---
MIT license