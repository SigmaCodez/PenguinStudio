# Platform connections and Wayland

## Twitch

1. Register an application in the [Twitch developer console](https://dev.twitch.tv/console/apps) owned by the Penguin Studio maintainer. Choose the **Public** client type for the device authorization grant.
2. Open **Docks → Stream details → Twitch** and enter its public client ID.
3. Select **Connect account**. Penguin opens Twitch in your system browser and displays the device code. Complete the requested `channel:manage:broadcast` authorization there.
4. Enter a title and optionally a Twitch game/category ID, then select **Update stream details**.

The module validates the token after connection and periodically while connected, and refreshes it before expiration. Disconnect clears the local session; revoking the application itself is done in your Twitch account settings. The official [device grant documentation](https://dev.twitch.tv/docs/authentication/getting-tokens-oauth/#device-code-grant-flow) and [channel update API](https://dev.twitch.tv/docs/api/reference/#modify-channel-information) define the required registration and permissions.

## YouTube

1. Create a Google Cloud project, enable the **YouTube Data API v3**, configure its OAuth consent screen, and register an OAuth client of type **Desktop app**. While the consent screen is in testing, add the accounts that will test Penguin Studio.
2. Open **Docks → Stream details → YouTube** and enter the desktop client ID and its client secret if supplied by Google. The secret is kept only in memory. Desktop client secrets are not a substitute for server-side credential security.
3. Select **Connect account** and complete authorization in the system browser. Penguin requests the YouTube scope required to edit video metadata. The callback binds only to `127.0.0.1` on an ephemeral port, uses random state and verifies a PKCE challenge.
4. Create or schedule a broadcast in [YouTube Studio](https://studio.youtube.com), load your broadcasts in Penguin, select one, and update its title.

The implementation updates the video's title while preserving its existing description, tags, category and language fields. It does not create broadcasts, enable live streaming on an account, or bypass Google's consent/API verification process. See [Google desktop OAuth](https://developers.google.com/identity/protocols/oauth2/native-app) and [videos.update](https://developers.google.com/youtube/v3/docs/videos/update).

## Other destinations

The first-launch wizard includes Twitch, YouTube, Kick, Facebook Live, TikTok LIVE and custom RTMP(S). Open the selected creator dashboard, then enter the server and stream key provided for that account. Some providers require access approval or account eligibility before exposing stream keys. No universal OAuth or metadata API exists across all these services.

## Wayland

Platform metadata controls use Qt Network and the external browser, so they do not rely on the embedded Twitch browser panel that OBS disables under Wayland. Screen capture continues to use OBS's PipeWire source and the desktop's screen-sharing portal. Install a portal backend appropriate to the desktop (for example GNOME, KDE or wlroots). Test the actual compositor, graphics driver and sandbox package being shipped.

## Secret handling

Do not commit tokens, stream keys or client secrets. The new **Broadcast destinations** and **Stream details** docks save only public application IDs and nonsecret destination configuration. Their access/refresh tokens, desktop client secrets and stream keys are session-only. A desktop keyring integration is required before these docks can offer persistent sign-in. The inherited OBS **Settings → Stream** workflow remains separate and can persist stream credentials in the local profile, including `service.json`. Real login and metadata tests require registered applications and authorized platform accounts; successful compilation alone does not verify those flows.
