# Stream chat

The next Penguin Studio release adds native Twitch and YouTube chat tabs. They
work under Wayland without an embedded browser. Connect the corresponding account
in **Stream details**, then open **Docks → Stream chat**.

Messages are rendered as plain text with role labels and bounded history. The
view follows new messages while you are at the bottom; scrolling up keeps your
reading position. Right-click a message for moderation. Destructive actions ask
for confirmation and are applied only when the platform accepts the request.

## Twitch

- Receive and send messages with EventSub and the Twitch chat API.
- Delete a message, apply a ten-minute timeout, ban or unban a participant.
- Change emote-only, follower-only, subscriber-only, slow and unique-message modes.
- Search connected chat participants or recent chatters and display the live viewer count.

Existing accounts can continue reading and sending. Reconnect Twitch once to
grant the additional moderation and participant-list permissions. A rejected
moderation action does not sign the account out.

**Chat participants are not all viewers.** Twitch's list identifies users connected
to chat and updates with a delay. Anonymous viewers and people watching without a
chat connection are not identified. The stream viewer count is a separate metric.

## YouTube

- Select an active broadcast with live chat, receive and send messages.
- Display owner, moderator and member labels.
- Delete messages, apply ten-minute timeouts and ban participants.
- Remove bans created during the current Penguin Studio chat session. Other bans
  must be managed in YouTube Studio because their ban identifiers are not available
  to this session.
- Search people observed posting in the last 15 minutes and display the concurrent
  viewer count when YouTube makes it available.

No chat appears until an active broadcast exposes a live chat. Permission,
quota and ended-chat errors stop polling instead of repeatedly using quota.
Temporary network failures retry with backoff. Message polling respects the
server interval and uses a minimum ten-second interval; it is not instantaneous.
Live viewer counts refresh once a minute. YouTube does not provide a complete
named viewer/presence list through this integration.

## Platform-specific features

**Dashboard ↗** opens the official Twitch or YouTube dashboard for
features not implemented natively, including platform-specific monetization,
advanced AutoMod workflows, polls and management of older YouTube bans. This is
not a claim of feature parity with either platform's website. Twitch emote codes
remain text; this native view does not download third-party emote images.

Signing out clears message history and participant data. Tokens continue to use
the existing system-keychain sign-in flow; chat history is not saved to disk.

## API references

- [Twitch API reference](https://dev.twitch.tv/docs/api/reference/)
- [Twitch chat moderation](https://dev.twitch.tv/docs/chat/moderation/)
- [YouTube live chat messages](https://developers.google.com/youtube/data/live/reference/rest/v3/liveChatMessages)
- [YouTube live chat bans](https://developers.google.com/youtube/data/live/reference/rest/v3/liveChatBans)
