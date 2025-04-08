# Contributing To GodotSteam

If you are interested in helping work on GodotSteam, here are the  major repositories for the project.

- [Main Project](https://github.com/GodotSteam/GodotSteam/)
- [Server](https://github.com/GodotSteam/GodotSteam-Server/)
- [MultiplayerPeer](https://github.com/GodotSteam/MultiplayerPeer/)
- [Documentation](https://github.com/GodotSteam/GodotSteam-Docs/)
- [Skillet](https://github.com/GodotSteam/Skillet)

## Modifying Code

One of the best way to help with the project is code contribution. [We keep a somewhat up-to-date list of what needs done in our To-Do list on Github.](https://github.com/orgs/GodotSteam/projects/3) If you are making rather large changes, it is advised you [contact us on Discord](https://discord.gg/SJRSq6K) or open a [discussion in Github Discussions](https://github.com/GodotSteam/GodotSteam/discussions) about it before spending a lot of time rewriting things that might need more rewriting.

### Code Styles

When modifying the code to any of GodotSteam branches, there is a convention we try to keep. Honestly, it is a hodge-podge of styles:

- [x] All Steamworks function names retain Valve's style except the first letter is lowercase. Example: SetAchievements = setAchievements
- [x] Use snake-case for all other function names; similar to how Godot does it.
- [x] Use snake-case for all signal names, variables, and arguments.

### Compile It

Most importantly, we ask that you make sure your code compiles before submitting a pull-request. Run a quick:

**Godot 2.x, 3.x**

	scons platform=[your platform] production=yes tools=yes target=release_debug

**Godot 4.x**

	scons platform=[your platform] tools=yes target=editor

## Modifying Docs

Feel free to make any edits you think are needed: CSS changes, new tutorials, extra links, fixes for functions or signals, or even adding your own game to the "Games Using GodotSteam" list.  Any large changes should be discussed by the same routes as major code changes.

The documentation uses [MKDocs](https://www.mkdocs.org/) and [Material For MKDocs](https://squidfunk.github.io/mkdocs-material/); if you want to set up a local version of the docs to see your updates before pushing to repo, you will need to install those.

Try to make any new or edited content fit the same styles as the other pages. But if you have an eye for design and think of something better, please share that too!

## Modifying Skillet

[Our open-source, free-to-play game Skillet](https://store.steampowered.com/app/3013040/Skillet/) will be launching on Steam this year.  We hope to get community support for adding more stuff and bug fixing.  Since it is meant to be a fun and educational resource, we want to treat the code contributions similar.

Please try to keep most of the standard Godot conventions, use snake_case, double linebreak between functions, etc.

Don't shy away from explaining things in the comments. Since these are meant for people to learn and figure out how Steamworks functions, definitely pack in any information you think necessary.

## Thank You

Yes, a huge thank you for helping out with the project in any and all ways: contributions, donations, or just supporting your fellow developers!