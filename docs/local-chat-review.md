# Local chat and stream-details review

This unreleased change is based on Penguin Studio 0.7.2. The public release and its previews remain unchanged.

## Changes

- Load Twitch stream details after connecting or restoring an account and when opening its details tab.
- Refresh YouTube broadcasts and the selected video's title/category automatically. Preserve unsaved title/category edits and ignore outdated responses.
- Embed original Twitch and YouTube chat pages in separate persistent Qt WebEngine profiles. API credentials are never injected into website sessions; system browser cookies are not imported.
- Pause hidden web pages and stop the alternative API chat while the original chat is selected. The chat menu offers reload, open in browser and the existing integrated chat.
- Remove the redundant top-bar settings gear, separate details icon and dashboard button. Settings remain in the application/menu controls; stream details remain in the Details tab and Go Live menu.
- Keep the privacy action aligned with the main controls.

## Availability and limitations

Original web chat requires Qt WebEngine. Builds without it explicitly retain integrated API chat. The Ubuntu local package includes its runtime dependencies.

Website sign-in is separate from the connected streaming account. Google restricts embedded sign-in: use the existing browser-based API connection and integrated chat if an original YouTube page cannot authenticate. Chat must be enabled for the selected YouTube broadcast. No user-agent spoofing, browser-cookie import or authentication-policy bypass is used.

## Validation

Offline platform flow tests cover authentication, automatic metadata loading, preserving edits, overlapping refresh exclusion and callback shutdown. Wayland workspace tests passed 237 checks. Recording, loopback streaming, privacy, audio assistance and upload reporting passed 31 checks with full recording/stream decoding and clean shutdown.

Original Twitch chat rendered inside the complete application. The public YouTube test rendered the platform's disabled-chat notice; authenticated posting and moderation have not been verified and no messages were sent. Web pages can emit provider JavaScript diagnostics when signed out.

Private test artifacts and website screenshots remain outside the repository. They are not current public product previews.

## Local2: YouTube broadcast identity

The authenticated app diagnostic found that the selected broadcast and embedded URL used an older video, while the supplied browser link used the intended video. The new local build requests broadcast lifecycle status, prefers live/testing/upcoming broadcasts over completed broadcasts when no selection can be restored, and stores explicit selections for restart. Saved IDs are used only if present in the authorized broadcast list. The selected video ID is visible beneath the broadcast selector and the same ID drives the original chat.

Regression coverage includes a live broadcast on a later API result page, restoring an explicit older selection, ignoring a missing saved ID, persisting a new explicit selection, and matching the visible ID to the chat callback. No user video IDs or chat messages are included in these fixtures.

The installed local2 package passed 237 Wayland workspace checks and clean shutdown in the isolated test. With the existing user profile, the intended authorized broadcast was found and selected; the loaded web chat video ID matched the selector and the disabled-chat notice was absent. Authenticated YouTube posting and moderation remain separate, unverified checks. No GitHub publication was performed.

## Local3: supported YouTube chat actions

The original website's Google sign-in opens the external browser and does not establish an embedded website session. An explicit explanation now offers integrated chat or the original chat in the external browser. Embedded Google navigation stays blocked; browser cookies are not imported. The integrated chat now requests the exact broadcast selected in Stream details instead of filtering only active broadcasts. Account disconnect clears the chat selection.

Offline chat-action and account-flow tests pass, including the exact selected-broadcast query without an active-only filter. A Wayland UI test confirms the sign-in explanation and switch to integrated chat. The workspace passed 237 checks and clean shutdown. Real YouTube sending and moderation remain pending live account verification.

Installed local3 verification with the existing account confirmed the selected broadcast, `Connected · YouTube live chat`, an enabled Send control, and native Wayland startup without shader/pipeline errors. Sending and deleting the explicit test message await user verification. This confirms API chat connectivity, not a Google login session in the embedded website.

## Confirmed live account result (local3)

The user confirmed successful posting of the designated test message through integrated YouTube chat and its visibility in the normal YouTube browser chat. Deleting only that test message through Penguin Studio removed it from both views. This validates the tested account's restored API connection, sending and individual-message deletion. It does not establish embedded Google website sign-in, all moderation operations, or eligibility of every Google account for the publisher OAuth application. The installed version remains 0.7.2-1+ubuntu26.04.local3; nothing was published to GitHub.

## Local4: remaining moderation checks

The user has no separate test accounts. No real viewers were timed out or banned. Expanded in-memory tests cover Twitch and YouTube timeout, permanent ban and unban payloads; invalid/empty actions; cancellation; duplicate request exclusion; failed unban state retention; missing YouTube ban IDs; session preservation after errors; and Twitch chat settings with enabled/disabled duration fields. These are simulated platform responses, not proof of live ban permissions.

Fixes reject unsupported actions and missing message/user identifiers, update Twitch message history immediately after confirmed deletion/ban, preserve failed moderation state, explain missing YouTube undo IDs, and distinguish quota, authorization, protected-target, missing-target, conflict and uncertain-network outcomes. No automatic moderation retries are introduced. Original platform website moderation remains controlled by Twitch/YouTube.

Local4 installation completed and the installed platform module matches the validated package byte for byte. The expanded moderation suite passed under Wayland; the package workspace passed all 237 checks, exited with code 0 and reported zero OBS memory leaks. Live timeout/ban/unban testing remains unperformed because no separate test accounts are available.
