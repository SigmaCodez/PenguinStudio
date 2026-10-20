# Platform connections and Wayland

## Twitch

1. Open **Docks → Stream details → Twitch** and select **Connect with Twitch**.
2. Penguin opens the official Twitch activation page in your system browser. Enter the displayed device code and authorize the application. No Twitch password is entered into Penguin Studio.
3. Use **Load current stream details** to retrieve the current title and category ID, or enter a title and optional category ID, then select **Update stream details**.
4. Open **Docks → Twitch chat**. The native dock reads and sends messages in the connected account's channel on Wayland as well as X11. It follows chat deletion/clear events and reconnects after connection loss. The current implementation displays plain text, including emote names; it does not embed Twitch's web dashboard or render third-party emote extensions.

The maintainer's public Twitch application ID is bundled with Penguin Studio. The application must remain registered with client type **Public**; no client secret is embedded. Device authorization requests `channel:manage:broadcast`, `user:read:chat` and `user:write:chat`. The module validates tokens after connection and periodically while connected, and refreshes them before expiration. Account tokens and chat history remain in memory for the session. Disconnect clears both. Revoking the application itself is done in Twitch account settings.

The chat uses Twitch EventSub WebSockets and the Send Chat Message API, with bounded history and authenticated HTTPS requests that do not follow redirects. An API failure is shown in the dock; it never falls back to sending unauthenticated messages. This login manages chat and metadata; broadcast destinations and their stream keys remain a separate setup step.

References: [device grant](https://dev.twitch.tv/docs/authentication/getting-tokens-oauth/#device-code-grant-flow), [channel update API](https://dev.twitch.tv/docs/api/reference/#modify-channel-information), [chat authentication](https://dev.twitch.tv/docs/chat/authenticating), [WebSocket lifecycle](https://dev.twitch.tv/docs/eventsub/handling-websocket-events/).

## YouTube

1. Create a Google Cloud project, enable the **YouTube Data API v3**, configure its OAuth consent screen, and register an OAuth client of type **Desktop app**. While the consent screen is in testing, add the accounts that will test Penguin Studio.
2. Open **Docks → Stream details → YouTube** and enter the desktop client ID and its client secret if supplied by Google. The secret is kept only in memory. Desktop client secrets are not a substitute for server-side credential security.
3. Select **Connect account** and complete authorization in the system browser. Penguin requests the YouTube scope required to edit video metadata. The callback binds only to `127.0.0.1` on an ephemeral port, uses random state and verifies a PKCE challenge.
4. Create or schedule a broadcast in [YouTube Studio](https://studio.youtube.com), load your broadcasts in Penguin, select one, and update its title.

The implementation updates the video's title while preserving its existing description, tags, category and language fields. It does not create broadcasts, enable live streaming on an account, or bypass Google's consent/API verification process. See [Google desktop OAuth](https://developers.google.com/identity/protocols/oauth2/native-app) and [videos.update](https://developers.google.com/youtube/v3/docs/videos/update).

## Other destinations

The first-launch wizard includes Twitch, YouTube, Kick, Facebook Live, TikTok LIVE and custom RTMP(S). Open the selected creator dashboard, then enter the server and stream key provided for that account. Some providers require access approval or account eligibility before exposing stream keys. No universal OAuth or metadata API exists across all these services.

## Wayland

Platform metadata controls and the native Twitch chat use Qt Network/WebSockets and system-browser authorization, so they do not rely on the embedded Twitch browser panel that OBS disables under Wayland. Screen capture continues to use OBS's PipeWire source and the desktop's screen-sharing portal. Install a portal backend appropriate to the desktop (for example GNOME, KDE or wlroots). Test the actual compositor, graphics driver and sandbox package being shipped.

## Secret handling

Do not commit tokens, stream keys or client secrets. The new **Broadcast destinations** and **Stream details** docks save only public application IDs and nonsecret destination configuration. Their access/refresh tokens, desktop client secrets and stream keys are session-only. A desktop keyring integration is required before these docks can offer persistent sign-in. The inherited OBS **Settings → Stream** workflow remains separate and can persist stream credentials in the local profile, including `service.json`. Real login and metadata tests require registered applications and authorized platform accounts; successful compilation alone does not verify those flows.
