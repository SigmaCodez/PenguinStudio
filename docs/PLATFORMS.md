# Platform connections and Wayland

## Twitch

1. Open **Docks → Stream details → Twitch** and select **Connect with Twitch**.
2. Penguin opens the official Twitch activation page in your system browser. Enter the displayed device code and authorize the application. No Twitch password is entered into Penguin Studio.
3. Use **Load current stream details** to retrieve the current title and category name, or enter a title and select an optional category from the searchable game list, then select **Update stream details**.
4. Open **Docks → Twitch chat**. The native dock reads and sends messages in the connected account's channel on Wayland as well as X11. It follows chat deletion/clear events and reconnects after connection loss. The current implementation displays plain text, including emote names; it does not embed Twitch's web dashboard or render third-party emote extensions.

The maintainer's public Twitch application ID is bundled with Penguin Studio. The application must remain registered with client type **Public**; no client secret is embedded. Device authorization requests `channel:manage:broadcast`, `user:read:chat` and `user:write:chat`. The module validates tokens after connection and periodically while connected, and refreshes them before expiration. Access tokens and chat history remain in memory. Refresh tokens are saved in the system keyring and exchanged for a new access token on startup. Rotated refresh tokens replace the saved value. Disconnect disables automatic sign-in and deletes the saved credentials. Revoking the application itself is done in Twitch account settings.

The chat uses Twitch EventSub WebSockets and the Send Chat Message API, with bounded history and authenticated HTTPS requests that do not follow redirects. An API failure is shown in the dock; it never falls back to sending unauthenticated messages. This login manages chat and metadata; broadcast destinations and their stream keys remain a separate setup step.

References: [device grant](https://dev.twitch.tv/docs/authentication/getting-tokens-oauth/#device-code-grant-flow), [channel update API](https://dev.twitch.tv/docs/api/reference/#modify-channel-information), [chat authentication](https://dev.twitch.tv/docs/chat/authenticating), [WebSocket lifecycle](https://dev.twitch.tv/docs/eventsub/handling-websocket-events/).

## YouTube

1. Create a Google Cloud project, enable the **YouTube Data API v3**, configure its OAuth consent screen, and register an OAuth client of type **Desktop app**. While the consent screen is in testing, add the accounts that will test Penguin Studio.
2. Open **Docks → Stream details → YouTube** and enter the desktop client ID and its client secret if supplied by Google. The secret and refresh token are saved together in the system keyring after successful authorization. Desktop client secrets are not a substitute for server-side credential security.
3. Select **Connect account** and complete authorization in the system browser. Penguin requests the YouTube scope required to edit video metadata. The callback binds only to `127.0.0.1` on an ephemeral port, uses random state and verifies a PKCE challenge.
4. Create or schedule a broadcast in [YouTube Studio](https://studio.youtube.com), load your broadcasts in Penguin, select one, and update its title.

The implementation updates the video's title and an optionally selected video category while preserving its description, tags and language fields. Categories are loaded from YouTube's videoCategories API for the desktop locale's region (US fallback) and filtered as you type. Only assignable categories are offered. The public Data API does not expose a writable individual game-title selector; choose specific games in YouTube Studio. Leaving the category field empty preserves the video's existing category. It does not create broadcasts, enable live streaming on an account, or bypass Google's consent/API verification process. See [Google desktop OAuth](https://developers.google.com/identity/protocols/oauth2/native-app) and [videos.update](https://developers.google.com/youtube/v3/docs/videos/update).

## Other destinations

The first-launch wizard includes Twitch, YouTube, Kick, Facebook Live, TikTok LIVE and custom RTMP(S). Open the selected creator dashboard, then enter the server and stream key provided for that account. Some providers require access approval or account eligibility before exposing stream keys. No universal OAuth or metadata API exists across all these services.

## Wayland

Platform metadata controls and the native Twitch chat use Qt Network/WebSockets and system-browser authorization, so they do not rely on the embedded Twitch browser panel that OBS disables under Wayland. Screen capture continues to use OBS's PipeWire source and the desktop's screen-sharing portal. Install a portal backend appropriate to the desktop (for example GNOME, KDE or wlroots). Test the actual compositor, graphics driver and sandbox package being shipped.

## Secret handling

Do not commit tokens, stream keys or client secrets. The **Stream details** dock stores refresh tokens and required desktop client secrets through QtKeychain in the system keyring (Secret Service or KWallet). Plaintext fallback is disabled. Public client IDs and an automatic-sign-in preference remain in local settings. If the keyring is locked or unavailable, an explicit message explains that persistence failed; the current connection can still be used. Disconnect disables future automatic sign-in even if keyring deletion fails, and allows retrying deletion. Closing Penguin Studio does not delete a successfully saved account. Stored accounts are isolated by application configuration path and provider.

Flatpak grants access to Secret Service and KWallet. A locally installed strict Snap may require connecting its password-manager-service interface with `sudo snap connect penguin-studio:password-manager-service`. No keyring password is collected by Penguin Studio.

The **Broadcast destinations** dock still keeps stream keys session-only. The inherited **Settings → Stream** workflow is separate and can persist stream credentials in the local profile, including `service.json`.

## Optional sign-in during setup

The first-start connection assistant offers Twitch and YouTube sign-in, with an explicit option to skip by continuing. The destination configuration page also has an optional account button. These open the same native account controls as the Stream details dock, so there is one account state per provider. Closing the account dialog returns to the wizard. Twitch starts browser authorization directly; YouTube lets users enter their own registered desktop client settings before connecting. Account authorization does not itself configure a video-stream destination or start broadcasting.
