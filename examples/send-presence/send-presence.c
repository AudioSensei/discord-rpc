/*
    This is a simple example in C of using the rich presence API asynchronously.
*/

#define _CRT_SECURE_NO_WARNINGS /* thanks Microsoft */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "discord_rpc.h"

static const char* APPLICATION_ID = "345229890980937739";
static int FrustrationLevel = 0;
static int64_t StartTime;
static int SendPresence = 1;
static int Data = 1;

static int prompt(char* line, size_t size)
{
    int res;
    char* nl;
    printf("\n> ");
    fflush(stdout);
    res = fgets(line, (int)size, stdin) ? 1 : 0;
    line[size - 1] = 0;
    nl = strchr(line, '\n');
    if (nl) {
        *nl = 0;
    }
    return res;
}

static void updateDiscordPresence()
{
    if (SendPresence) {
        char buffer[256];
        DiscordRichPresence discordPresence;
        memset(&discordPresence, 0, sizeof discordPresence);
        discordPresence.state = "West of House";
        sprintf(buffer, "Frustration level: %d", FrustrationLevel);
        discordPresence.details = buffer;
        discordPresence.startTimestamp = StartTime;
        discordPresence.endTimestamp = time(0) + 5 * 60;
        discordPresence.largeImageKey = "canary-large";
        discordPresence.smallImageKey = "ptb-small";
        discordPresence.partyId = "party1234";
        discordPresence.partySize = 1;
        discordPresence.partyMax = 6;
        discordPresence.partyPrivacy = DISCORD_PARTY_PUBLIC;
        discordPresence.matchSecret = "xyzzy";
        discordPresence.joinSecret = "join";
        discordPresence.spectateSecret = "look";
        discordPresence.instance = 0;
        Discord_UpdatePresence(&discordPresence);
    }
    else {
        Discord_ClearPresence();
    }
}

static void handleDiscordReady(const DiscordUser* connectedUser, void* userData)
{
    printf("\nDiscord: connected to user %s#%s - %s on pipe %i\n",
           connectedUser->username,
           connectedUser->discriminator,
           connectedUser->userId,
           Discord_GetUsedPipeId());
    if (*(int*)userData == Data) {
        printf("\nUserData correct\n");
    }
    else {
        printf("\nUserData not correct\n");
    }
}

static void handleDiscordDisconnected(int errcode, const char* message, void* userData)
{
    printf("\nDiscord: disconnected (%d: %s)\n", errcode, message);
    if (*(int*)userData == Data) {
        printf("\nUserData correct\n");
    }
    else {
        printf("\nUserData not correct\n");
    }
}

static void handleDiscordError(int errcode, const char* message, void* userData)
{
    printf("\nDiscord: error (%d: %s)\n", errcode, message);
    if (*(int*)userData == Data) {
        printf("\nUserData correct\n");
    }
    else {
        printf("\nUserData not correct\n");
    }
}

static void handleDiscordJoin(const char* secret, void* userData)
{
    printf("\nDiscord: join (%s)\n", secret);
    if (*(int*)userData == Data) {
        printf("\nUserData correct\n");
    }
    else {
        printf("\nUserData not correct\n");
    }
}

static void handleDiscordSpectate(const char* secret, void* userData)
{
    printf("\nDiscord: spectate (%s)\n", secret);
    if (*(int*)userData == Data) {
        printf("\nUserData correct\n");
    }
    else {
        printf("\nUserData not correct\n");
    }
}

static void handleDiscordJoinRequest(const DiscordUser* request, void* userData)
{
    int response = -1;
    char yn[4];
    printf("\nDiscord: join request from %s#%s - %s\n",
           request->username,
           request->discriminator,
           request->userId);
    if (*(int*)userData == Data) {
        printf("\nUserData correct\n");
    }
    else {
        printf("\nUserData not correct\n");
    }
    do {
        printf("Accept? (y/n)");
        if (!prompt(yn, sizeof yn)) {
            break;
        }

        if (!yn[0]) {
            continue;
        }

        if (yn[0] == 'y') {
            response = DISCORD_REPLY_YES;
            break;
        }

        if (yn[0] == 'n') {
            response = DISCORD_REPLY_NO;
            break;
        }
    } while (1);
    if (response != -1) {
        Discord_Respond(request->userId, response);
    }
}

static void discordInit()
{
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof handlers);
    handlers.userData = &Data;
    handlers.ready = handleDiscordReady;
    handlers.disconnected = handleDiscordDisconnected;
    handlers.errored = handleDiscordError;
    handlers.joinGame = handleDiscordJoin;
    handlers.spectateGame = handleDiscordSpectate;
    handlers.joinRequest = handleDiscordJoinRequest;
    Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL, 0);
}

static void gameLoop()
{
    char line[512];
    char* space;

    StartTime = time(0);

    printf("You are standing in an open field west of a white house.\n");
    while (prompt(line, sizeof line)) {
        if (line[0]) {
            if (line[0] == 'q') {
                break;
            }

            if (line[0] == 't') {
                printf("Shutting off Discord.\n");
                Discord_Shutdown();
                continue;
            }

            if (line[0] == 'c') {
                if (SendPresence) {
                    printf("Clearing presence information.\n");
                    SendPresence = 0;
                }
                else {
                    printf("Restoring presence information.\n");
                    SendPresence = 1;
                }
                updateDiscordPresence();
                continue;
            }

            if (line[0] == 'y') {
                printf("Reinit Discord.\n");
                discordInit();
                continue;
            }

            if (time(NULL) & 1) {
                printf("I don't understand that.\n");
            }
            else {
                space = strchr(line, ' ');
                if (space) {
                    *space = 0;
                }
                printf("I don't know the word \"%s\".\n", line);
            }

            ++FrustrationLevel;

            updateDiscordPresence();
        }

#ifdef DISCORD_DISABLE_IO_THREAD
        Discord_UpdateConnection();
#endif
        Discord_RunCallbacks();
    }
}

int main(int argc, char* argv[])
{
    discordInit();

    gameLoop();

    Discord_Shutdown();
    return 0;
}
