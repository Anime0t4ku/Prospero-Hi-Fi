# Prospero Hi-Fi

A music player for PS5 homebrew, based on **NX Hi-Fi** and **MiSTer Hi-Fi**.

Play music from internal storage, M.2, USB, SMB network shares and external USB audio CD drives. Includes album artwork, a five-band equalizer, shuffle, repeat, a fullscreen player and remote control from a browser or NX Hi-Fi.

**Supported formats:** MP3, FLAC, PCM WAV and Ogg Vorbis.

## Installation

Requires a working PS5 homebrew setup with **ShadowMount+**.

1. Download and extract the ready-to-use homebrew folder ZIP.
2. Copy the complete `PPSA99287` folder to `/data/homebrew/PPSA99287/`.
3. Scan/register it in ShadowMount+ and launch **Prospero Hi-Fi**.
4. Add music to a folder below, open its source and select a track.

To update, replace the complete application folder and rescan. Your settings remain in `/data/prospero-hifi/`.

## Music sources

| Storage | Default folder |
| --- | --- |
| Internal Storage | `/data/music` |
| M.2 Storage | `/mnt/ext1/Music` |
| USB Storage | `/mnt/usb0/Music` through `/mnt/usb7/Music` |
| USB Extended Storage | `/mnt/ext0/Music` |

Create the external drive's `Music` folder if needed. Highlight a source and press **Triangle** to change its music folder.

Internal Storage is always listed. M.2 and USB sources appear when their drives are mounted and accessible. Physical Disc appears when a supported USB optical drive is detected.

**Network shares:** choose **Sources > Add SMB share**, enter the server, share and optional subfolder, then test and save. For `\\NAS\Music\Albums`, use server `NAS`, share `Music` and subfolder `Albums`. SMB2/SMB3, guest access and username/password login are supported.

**Audio CDs:** connect an external USB CD/DVD drive, insert an audio CD and open **Physical Disc**. CDs play directly, with seeking, EQ and gapless playback. MusicBrainz provides track information when available; unknown discs use numbered track names. Reopen Physical Disc after changing CDs. The PS5's built-in disc drive is not supported.

## Controls

| Button | Action |
| --- | --- |
| D-pad / left stick | Navigate |
| ✕ / ○ | Select / Back |
| L1 / R1 | Switch tabs; previous/next track in fullscreen |
| L2 / R2 | Previous / Next track |
| Options | Play / Pause |
| Square | Toggle shuffle |
| Triangle | Edit a source; otherwise stop playback |
| Touchpad click | Switch between browsing and mini-player controls |
| R3 | Open / Close fullscreen |

In either player, **Up/Down** moves between control rows. **Left/Right** selects buttons, seeks in ten-second steps or adjusts volume in five-percent steps, depending on the highlighted control. **Circle** leaves player controls or closes fullscreen.

In **Settings**, highlight a category on the left and press **✕** to enter its settings. Press **○** to return to categories. The mini player stays on the right.

On the **on-screen keyboard**, use **✕** to type, **Square** to delete, **Triangle** for a space, **L1/R1** to change pages, **Options** to confirm and **○** to cancel.

## Playback and appearance

Selecting a track starts playback of that folder's supported music files. Keep browsing while music plays, or open fullscreen with **R3**. Enable **Settings > Playback > Auto Fullscreen** to open it automatically when selecting a track; this is off by default.

Choose Dark, Light or OLED themes, an accent colour, artwork preferences, a visualizer, a clock and a screensaver. Blue is the default accent. The interface scales to the active output resolution, and settings save automatically.

Artwork comes from supported MP3/FLAC tags or nearby JPG/PNG files such as `cover.jpg`, `folder.png` or `front.jpg`.

## Remote control

Enable **Web Remote** in Settings. Keep Prospero running and connect both devices to the same network.

**Browser:** open the address shown in Settings, normally `http://PS5-IP:8182`. Browse music, control playback, view artwork, adjust EQ/settings and manage sources. **Screen Control** mirrors the PS5 interface and supports remote text entry.

**NX Hi-Fi:** open **Remote** on your Switch and add the PS5's IP address with port **8182**, or scan for clients. Choose **Independent control** for NX's own remote interface or **Screen Control** to operate Prospero's screen. Older NX clients need Screen Control to access the separate M.2/USB entries. In Screen Control, hold **Minus for three seconds** to open NX's local menu and disconnect.

Audio plays on the PS5. Disconnecting a remote leaves playback running. Prospero accepts remote control but cannot control another Hi-Fi instance.

## Notes

- Prospero must remain open during playback; background music during PS5 games is not supported.
- Local M4A/AAC/ALAC and Opus playback is not supported.
- Configuration is stored in `/data/prospero-hifi/config.json` and may contain network passwords. Do not share your personal config file.

## Building

On Linux or WSL with the required build tools:

```sh
bash scripts/bootstrap-ps5.sh
export PS5_PAYLOAD_SDK="$PWD/.deps/ps5-payload-sdk"
make -f Makefile.ps5 -j4
```

The complete homebrew folder is generated in `dist/PPSA99287/`.

## Credits and license

Developed by **Anime0t4ku**, based on **NX Hi-Fi** and **MiSTer Hi-Fi**, with PS5SX2-derived keyboard and USB optical-drive work.

Released under [GPLv3](LICENSE). See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for component credits and licenses. Music and artwork are not included.
