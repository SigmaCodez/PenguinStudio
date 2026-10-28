# Stream chat

Penguin Studio 0.7.3 offers Twitch and YouTube tabs with original platform web views where Qt WebEngine is available. Open **Stream chat** from the Docks menu. Connect your account in **Details**; current metadata loads automatically without overwriting unsaved edits.

## Original platform chat

The chat menu offers **Reload chat**, **Open in browser**, and **Use integrated chat**. Website sessions are separate from API account connections and your normal browser. Website cookies stay in dedicated local profiles; API tokens and system-browser cookies are never injected into them. Hidden web pages pause to reduce background activity.

Twitch website sign-in and sending were confirmed in local user testing. Google sign-in in the normal browser does not sign in the embedded YouTube page. Penguin Studio explains this and offers integrated chat or the original chat in the browser. Do not interpret a successful API connection as an authenticated website session.

Ubuntu DEB, Fedora RPM, Snap and AppImage builds request Qt WebEngine. The current Flatpak build uses integrated API chat because its bundled Qt build does not include WebEngine. Build-time fallback is explicit; no package claims unsupported website functionality.

## YouTube broadcast selection

Choose the broadcast in **Details → YouTube**. The selected video ID appears beneath the selector. Explicit selections are remembered; when no selection can be restored, live/testing/upcoming broadcasts are preferred over completed broadcasts. Both chat views use the selected broadcast, including a scheduled broadcast if YouTube exposes its chat. Availability and permissions remain controlled by YouTube.

## Integrated chat and moderation

Choose **Use integrated chat** to use the account connected in Details. Messages are plain text with role labels and bounded history. Scroll up to retain your reading position. Right-click a message for moderation; actions require confirmation.

Twitch supports sending, deleting individual messages, ten-minute timeouts, permanent bans and removing bans/timeouts. Chat settings include emote-only, follower-only, subscriber-only, slow and unique-message modes. Participant lists identify chat connections, not everyone watching; anonymous viewers are not identified.

YouTube supports sending, individual-message deletion, ten-minute timeouts and permanent bans. Only bans with an identifier returned during the current session can be removed here; use YouTube Studio for older bans or missing identifiers. The app explains quota, authorization, protected-target, missing-target and conflicting-action failures. Uncertain results are not automatically retried.

YouTube polling respects the server interval and a ten-second minimum. Viewer counts refresh once a minute. YouTube provides no complete named viewer list; recent chatters are people observed posting within 15 minutes.

## Verification and limits

User testing confirmed integrated YouTube sending and deletion in both the app and browser. Automated in-memory tests cover timeouts, bans, unbans, cancellation, invalid inputs, errors, token refresh and Twitch chat settings under Wayland. No separate test accounts were available, so real timeout/ban/unban operations are not claimed as verified. Platform-specific monetization, advanced AutoMod, polls and other website features are not feature-parity promises.

API tokens use the system keyring. Disconnect clears API chat state. Website sign-in uses its separate web profile; sign out on the website to end that session. Chat history in the integrated view is not persisted.

## References

- [Twitch moderation](https://dev.twitch.tv/docs/chat/moderation/)
- [YouTube live chat API](https://developers.google.com/youtube/data/live/reference/rest/v3/liveChatMessages)
- [Google supported sign-in browsers](https://support.google.com/accounts/answer/7675428?hl=en)
