# Pinned open-source dependencies

Exact upstream URLs and commit IDs are in `sources.json`. Sources are included
for reproducible builds and relinking. No GTA client assets or proprietary server
executables are distributed in our deployment ZIPs.

- samp-cpp / SAMP EDGEngine: Rexrn, MIT; see `samp-cpp/LICENSE`.
- QuickMaffs: PoetaKodu, MIT; see `quickmaffs/LICENSE`.
- SAMPGDK: samp-forks mirror of Zeex's project, Apache-2.0; see `sampgdk/LICENSE.txt`.
- SA-MP Plugin SDK: udan11 fork; retain the notices in `samp-sdk/amx` and SDK sources.
- MariaDB Connector/C: LGPL-2.1; see `mariadb-connector/COPYING.LIB`. The static
  connector is linked into the plugin. Its complete source is here; the supplied
  CMake/Docker build allows replacing and relinking that library.
- Pawn compiler: pawn-lang/compiler; see `pawn-compiler/license.txt`. Only used to
  compile the tiny bootstrap AMX; it contains no gameplay.
- OpenSSL 3.5.9 LTS: built as a static i386 library from the official source archive
  pinned by URL/SHA256 in sources.json. Apache-2.0; see OpenSSL-COPYRIGHT.txt and
  OpenSSL-NOTICE.txt (upstream AUTHORS.md). The Dockerfile contains its verified download/build recipe.
- zlib and GCC 8 runtime: linked from the Ubuntu 18.04 build toolchain; see the
  additional copyright files included in the deployment archive and build recipe.

CityLive changes to upstream samp-cpp:
1. Check a vehicle lookup before dereferencing it in server callbacks; an unknown
   vehicle can be owned by a different plugin.
2. Guard the SAMPGDK_STATIC macro against duplicate definition.
3. Compile ServerCore's OnDialogResponse as CityFrameworkOnDialogResponse so the
   game can validate its own authentication dialogs before falling back to the
   framework. This rename is a per-source CMake definition.
4. Remove event-receiver hooks with a loop that remains valid when the dispatcher
   erases itself. Upstream iterated a vector while erasing from it, which could
   leave dangling callbacks and crash shutdown. A checked-iterator test covers it.
5. Release the server before SAMPGDK is unloaded, and clear its global game-mode
   pointer at game-mode exit. The database worker joins during game destruction.

The repository's other third-party sources are unmodified snapshots. Build-only
sources and unused client-auth plugin DLLs are not part of deployment ZIPs.

This is a server-side framework, not a replacement for GTA SA, an Android client,
or the Unreal Engine runtime. See the migration scope in ../README.md.
