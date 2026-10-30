# Prospero Hi-Fi

Prospero Hi-Fi is a music player for PS5 homebrew, based on my NX Hi-Fi project and its MiSTer Hi-Fi roots. It brings internal, M.2, USB and network music playback, physical audio CDs, album artwork, an equalizer and remote control from a browser or NX Hi-Fi.

Supported audio formats and codecs: **MP3, FLAC, PCM WAV and Ogg Vorbis**.

## Getting started

1. Extract the ready-to-use ShadowMount+ folder ZIP on your computer.
2. Copy the complete `PPSA99287` folder to `/data/homebrew/PPSA99287/` on your PS5. Keep all included files and subfolders together.
3. With your jailbreak, kstuff and ShadowMount+ active, scan/register the homebrew folder and launch **Prospero Hi-Fi** from the PS5 home screen.
4. Put music in the appropriate folder below, choose its source and select a song.

| Storage | Default music folder |
| --- | --- |
| Internal Storage | `/data/music` |
| M.2 Storage | `/mnt/ext1/Music` |
| USB Storage | `/mnt/usb0/Music` through `/mnt/usb7/Music` |
| USB Extended Storage | `/mnt/ext0/Music` |

Create the `Music` folder on an external drive if it does not exist. Select a storage source and press **Triangle** to choose a different folder. Each external mount remembers its own music folder.

Internal Storage is always visible. M.2 and USB entries appear only when the corresponding volume is mounted and accessible. Physical Disc appears only when a supported USB optical drive is detected, even if no CD is inserted. Drive detection refreshes in the background.

The current target setup is **PS5 firmware 13.60 with Y2JB and ShadowMount+**. Older firmware compatibility depends on the homebrew setup and has not been fully verified. If your setup does not already grant storage access, a local ELF loader must be listening on port **9021**; Prospero submits its bundled access helper automatically.

For network features, connect your PS5 through its system settings first. Existing settings are kept when you replace the application folder. Use the ready-to-use folder download for installation; the source ZIP is for development.

## Playing music

Play supported music files from Internal Storage, M.2 Storage, USB storage or an SMB network share.

Selecting a song plays the supported music files in that folder, starting from your selection. Use the player controls to pause, skip, stop, change volume, shuffle or repeat. Repeat cycles between Off, Repeat All and Repeat One.

You can keep browsing while music plays. Open the fullscreen player by pressing **R3** or selecting the expand button. Press **Circle** or **R3** to return.

The interface scales to the PS5's active output resolution. Prospero Hi-Fi must remain running to play music; it does not provide background music while playing PS5 games.

## Controls

| DualSense control | Action |
| --- | --- |
| L1 / R1 | Switch sections |
| D-pad / left stick | Move through lists |
| Cross / Circle | Select / Back |
| Left / Right | Jump through lists, adjust EQ, or select a player control |
| Options | Play / Pause |
| L2 / R2 | Previous / Next song |
| Square | Toggle shuffle |
| Touchpad click | Switch between browsing and player controls |
| R3 | Open or close fullscreen |
| Triangle | Edit the selected source; otherwise stop playback |

While player controls are focused, Up/Down seeks in five-second steps. On-screen hints show the controls available on each screen.

### On-screen keyboard

The controller keyboard includes lowercase, uppercase and symbol pages, plus Space, Delete and Done keys.

| Control | Action |
| --- | --- |
| D-pad / left stick | Select a key |
| Cross | Type the selected key |
| Square | Delete the previous character |
| Triangle | Insert a space |
| L1 / R1 | Change keyboard page |
| Options | Confirm |
| Circle | Cancel without saving changes |

## Music from a network share

Open **Sources > Add SMB share**, enter the server and share details, test the connection and select **Save share**.

For a network folder such as `\\NAS\Music\Albums`, enter:

- **Server:** `NAS` or its IP address
- **Share:** `Music`
- **Subfolder:** `Albums` — optional

Enter a username and password, or enable Guest if your server allows it. SMB2/SMB3 shares are supported. Select a saved share and press **Triangle** to edit or remove it.

## Artwork and appearance

Artwork is read from supported MP3 and FLAC files. You can also place `cover.jpg`, `folder.png` or `front.jpg` beside your music; JPG, JPEG and PNG variants are supported.

Settings includes Light, Dark and OLED themes, accent colours, artwork preferences, a visualizer, a clock and a screensaver. **Blue is the default accent colour.** The Equalizer section provides presets and manual adjustment of five frequency bands.

Settings are saved automatically. The screensaver turns the display black while playback continues; press a button to wake it.

## Control Prospero Hi-Fi from a browser

Enable **Web Remote** in Settings and open the displayed address on a phone or computer on the same network:

```text
http://PS5-IP:8182
```

The Web Remote controls playback on the PS5. It provides:

- Music browsing and playback from the detected storage sources, SMB shares and physical audio CDs.
- Play/pause, stop, previous/next, seeking, volume, shuffle and repeat.
- Now-playing information and album artwork.
- Equalizer presets and manual adjustment.
- Appearance, playback and other application settings.
- Storage-folder editing and SMB-share management.

The browser also offers **Screen Control**, which mirrors the current interface and lets you operate it remotely. Remote text prompts let you enter text when the PS5 opens a supported input dialog.

Prospero Hi-Fi must stay open for the Web Remote to work. Playback and audio stay on the PS5; the remote does not stream audio to your browser.

## Control Prospero Hi-Fi from NX Hi-Fi

On your Switch, open **NX Hi-Fi > Remote** and add the PS5 using its IP address and port **8182**, or choose **Scan for remote clients**. Prospero must be running with Web Remote enabled, and both devices must be on the same network.

Choose between:

- **Independent control:** browse and control playback on the PS5 through NX Hi-Fi's own remote interface. Playback controls, artwork, equalizer, supported settings and source management are available through the target's capabilities.
- **Screen Control:** follow and control Prospero's current on-screen interface, including its detected storage-source list and supported text-entry prompts.

Existing NX Hi-Fi clients use their original source list in Independent control. To access the separate M.2 and USB storage entries, use **Screen Control** or the browser Web Remote.

In NX Screen Control, hold **Minus for three seconds** to open the local menu. Use it to disconnect or switch modes. Disconnecting leaves music playing on the PS5.

Prospero accepts incoming remote control. It does not include an outgoing Remote section for controlling another Hi-Fi instance.

## Physical audio CDs

Connect a USB CD/DVD drive. Once **Sources > Physical Disc** appears, insert an audio CD and open it. Use a powered hub or a separately powered drive if additional power is needed.

The disc is streamed as CDDA (44.1 kHz, 16-bit stereo); no ripping or temporary audio files are required. Pause, seeking, previous/next, shuffle, repeat, EQ and the visualizer work with CDs. **Gapless audio CD playback** is enabled by default and can be changed in Settings. Data tracks on mixed-mode CDs are skipped.

MusicBrainz supplies album, artist and track names from the disc's table of contents. Artwork uses Cover Art Archive, then a matching iTunes album. Metadata and artwork are cached in `/data/prospero-hifi/cd-cache/`. Offline or unknown discs use Track 01-style names. Unknown-disc results expire after seven days; network/server failures are not cached.

To refresh after inserting or changing a disc, reopen Physical Disc from Sources. Read errors or drive removal stop playback and display an error. Physical Disc is also available through the browser and compatible NX remote clients.

This feature uses an external USB optical drive. Compatibility depends on the drive, USB bridge and firmware; the PS5's built-in disc drive is not supported as an audio CD source.

## Helpful notes

- **Local format support:** M4A/AAC/ALAC and Opus are not supported for local playback. Some files may have limited seeking or an estimated duration.
- **Missing drive:** check that the drive is connected, powered and accessible to homebrew. An empty mount directory alone does not make a storage source appear. A detected drive can be shown even if its Music folder is missing or empty.
- **Connection problems:** check that both devices are on the same network, the address is correct and Prospero's Web Remote is enabled. The default remote port is **8182**.
- **Settings and passwords:** configuration is stored in `/data/prospero-hifi/config.json`. It can contain saved network credentials, so do not share your personal configuration file. Leave a saved password field blank to retain it where indicated.
- **Upgrading:** replace the complete `PPSA99287` application folder and rescan ShadowMount+. Settings and CD metadata remain in `/data/prospero-hifi/`. A previously selected combined USB music path is migrated to its separate storage source.

## Building from source

See `scripts/bootstrap-ps5.sh` and `Makefile.ps5` for the public-SDK build. On Linux or WSL with the required build tools installed:

```sh
bash scripts/bootstrap-ps5.sh
export PS5_PAYLOAD_SDK="$PWD/.deps/ps5-payload-sdk"
make -f Makefile.ps5 -j4
```

The ready-to-use homebrew folder is generated in `dist/PPSA99287/`. See `VALIDATION.md` for build checks and console testing notes.

## License

Prospero Hi-Fi is developed by **Anime0t4ku** and released under [GPLv3](LICENSE). It is based on **NX Hi-Fi** and **MiSTer Hi-Fi**, with PS5SX2-derived keyboard and USB optical-drive work. Third-party components retain their own licenses and notices; see `THIRD_PARTY_NOTICES.md` and `licenses/`.

Music and artwork are not included.
