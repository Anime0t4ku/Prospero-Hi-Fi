# Prospero Hi-Fi

A PS5 homebrew music player by **Anime0t4ku**, based on NX Hi-Fi and MiSTer Hi-Fi.

Play MP3, FLAC, PCM WAV and Ogg Vorbis from internal storage, M.2, USB drives or SMB network shares. Play physical audio CDs using a supported external USB CD/DVD drive.

Features include album artwork, a five-band equalizer, shuffle, repeat, theme colors, fullscreen visualizers and remote control from a browser or NX Hi-Fi.

## Installation

Requires a PS5 homebrew setup with **ShadowMount+**.

1. Extract the homebrew folder ZIP.
2. Copy `PPSA99287` to `/data/homebrew/PPSA99287/`.
3. Scan/register it in ShadowMount+ and launch the app.

To update, replace the application folder. Settings are kept in `/data/prospero-hifi/`.

## Add music

- **Internal storage:** use `/data/music`.
- **M.2 or USB:** create a `Music` folder on the drive. Mounted drives appear under Sources.
- **Network:** select **Sources > Add SMB share** and enter your share details.
- **Audio CD:** connect a supported USB optical drive, insert a CD and open **Physical Disc**. Track information is retrieved from MusicBrainz when available.

Highlight a storage source and press **Triangle** to change its music folder. The PS5's built-in disc drive is not supported.

## Controls

| Button | Action |
| --- | --- |
| D-pad / left stick | Navigate |
| Cross / Circle | Select / Back |
| L1 / R1 | Switch tabs; previous/next track in fullscreen |
| L2 / R2 | Previous/next track in menus; seek five seconds in fullscreen |
| Options | Play / Pause |
| Square | Toggle shuffle |
| Triangle | Edit a source; otherwise stop playback |
| Touchpad click | Switch between browsing and player controls |
| R3 | Open / Close fullscreen player |
| L3 | Enter Listening Mode from fullscreen |

Follow the footer hints for the selected control. Open the sliders icon in the fullscreen player for Equalizer and Visualizer settings; use **L1/R1** to switch its tabs.

Highlight the visualizer and press **Cross** to expand it. **Left/Right** changes styles while highlighted or expanded; **Circle** returns to the player.

## Remote control

Enable **Web Remote** in Settings and connect both devices to the same network.

- **Browser:** open `http://PS5-IP:8182` using your PS5's IP address.
- **NX Hi-Fi:** scan for the PS5 or add its IP address with port **8182**.

Audio plays on the PS5. The mini player, browser and remote clients use the stock visualizer.

## Notes

Prospero must stay open during playback; music cannot continue in the background while playing PS5 games. Settings include themes, visualizer options and Listening Mode preferences.

## Building

On Linux or WSL with the required build tools:

```sh
bash scripts/bootstrap-ps5.sh
export PS5_PAYLOAD_SDK="$PWD/.deps/ps5-payload-sdk"
make -f Makefile.ps5 -j4
```

The application folder is generated in `dist/PPSA99287/`.

## License

Released under [GPLv3](LICENSE). See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for component credits and licenses.
