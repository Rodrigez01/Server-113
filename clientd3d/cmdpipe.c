// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * cmdpipe.c: Named Pipe for external command injection
 *
 * This module implements a Named Pipe server in the client that allows
 * external tools (like admin tools) to inject commands into the client
 * text input as if the user typed them. This replaces unreliable sendkeys.
 *
 * HOW IT WORKS:
 * 1. Client creates a Named Pipe on startup: \\.\pipe\Meridian59_Command_<PID>
 * 2. External tools connect to this pipe and send commands
 * 3. Commands are queued and executed during the main game loop
 * 4. Commands are sent to the server as if user typed them
 *
 * SECURITY:
 * - Pipe is only accessible from local machine
 * - Only accepts connections from same user account
 * - Limited command buffer size to prevent overflow
 */

#include "client.h"
#include "cmdpipe.h"

#ifdef __cplusplus
extern "C" {
#endif

// Pipe state
static HANDLE hPipe = INVALID_HANDLE_VALUE;
static HANDLE hThread = NULL;
static BOOL bRunning = FALSE;
static CRITICAL_SECTION csCommandQueue;

// Command queue
typedef struct CommandNode {
    char command[CMDPIPE_MAX_COMMAND + 1];
    struct CommandNode *next;
} CommandNode;

static CommandNode *pCommandHead = NULL;
static CommandNode *pCommandTail = NULL;

// Forward declarations
static DWORD WINAPI CommandPipeThread(LPVOID lpParam);
static void EnqueueCommand(const char *command);
static char* DequeueCommand(void);
static void ProcessCommand(const char *command);

/************************************************************************/
/*
 * CommandPipeInit: Initialize the command pipe system
 */
void CommandPipeInit(void)
{
    char pipeName[256];
    DWORD dwThreadId;

    // Initialize critical section for thread-safe queue
    InitializeCriticalSection(&csCommandQueue);

    // Create pipe name with process ID to allow multiple clients
    sprintf(pipeName, CMDPIPE_NAME, GetCurrentProcessId());

    debug(("CommandPipe: Initializing pipe: %s\n", pipeName));

    // Create named pipe
    hPipe = CreateNamedPipe(
        pipeName,                       // Pipe name
        PIPE_ACCESS_INBOUND,            // Read-only pipe
        PIPE_TYPE_MESSAGE |             // Message-type pipe
        PIPE_READMODE_MESSAGE |         // Message-read mode
        PIPE_WAIT,                      // Blocking mode
        1,                              // Max instances (only 1 connection at a time)
        CMDPIPE_BUFFER_SIZE,            // Output buffer size
        CMDPIPE_BUFFER_SIZE,            // Input buffer size
        CMDPIPE_TIMEOUT,                // Client timeout
        NULL                            // Default security (same user only)
    );

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        debug(("CommandPipe: Failed to create pipe, error = %d\n", GetLastError()));
        return;
    }

    debug(("CommandPipe: Pipe created successfully\n"));

    // Start listener thread
    bRunning = TRUE;
    hThread = CreateThread(
        NULL,                   // Default security
        0,                      // Default stack size
        CommandPipeThread,      // Thread function
        NULL,                   // Parameter
        0,                      // Creation flags
        &dwThreadId            // Thread ID
    );

    if (hThread == NULL)
    {
        debug(("CommandPipe: Failed to create thread\n"));
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
        DeleteCriticalSection(&csCommandQueue);
        return;
    }

    debug(("CommandPipe: Listener thread started\n"));
}

/************************************************************************/
/*
 * CommandPipeClose: Shutdown the command pipe system
 */
void CommandPipeClose(void)
{
    CommandNode *pNode;

    if (hThread == NULL && hPipe == INVALID_HANDLE_VALUE)
        return;

    debug(("CommandPipe: Shutting down\n"));

    // Signal thread to stop
    bRunning = FALSE;

    // Close pipe to unblock the thread
    if (hPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
    }

    // Wait for thread to finish
    if (hThread != NULL)
    {
        WaitForSingleObject(hThread, 5000);
        CloseHandle(hThread);
        hThread = NULL;
    }

    // Clean up command queue
    EnterCriticalSection(&csCommandQueue);
    while (pCommandHead != NULL)
    {
        pNode = pCommandHead;
        pCommandHead = pNode->next;
        SafeFree(pNode);
    }
    pCommandTail = NULL;
    LeaveCriticalSection(&csCommandQueue);

    DeleteCriticalSection(&csCommandQueue);

    debug(("CommandPipe: Shutdown complete\n"));
}

/************************************************************************/
/*
 * CommandPipeThread: Background thread that listens for pipe connections
 */
static DWORD WINAPI CommandPipeThread(LPVOID lpParam)
{
    char buffer[CMDPIPE_BUFFER_SIZE];
    DWORD bytesRead;
    BOOL connected;

    debug(("CommandPipe: Listener thread running\n"));

    while (bRunning)
    {
        // Wait for a client to connect
        debug(("CommandPipe: Waiting for connection...\n"));
        connected = ConnectNamedPipe(hPipe, NULL);

        if (!connected && GetLastError() != ERROR_PIPE_CONNECTED)
        {
            if (bRunning)  // Only log error if we're still supposed to be running
                debug(("CommandPipe: ConnectNamedPipe failed, error = %d\n", GetLastError()));
            break;
        }

        debug(("CommandPipe: Client connected\n"));

        // Read commands from client
        while (bRunning)
        {
            bytesRead = 0;
            if (!ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL))
            {
                if (GetLastError() == ERROR_BROKEN_PIPE)
                {
                    debug(("CommandPipe: Client disconnected\n"));
                    break;
                }
                debug(("CommandPipe: ReadFile failed, error = %d\n", GetLastError()));
                break;
            }

            if (bytesRead > 0)
            {
                // Null-terminate the command
                buffer[bytesRead] = '\0';

                // Remove trailing newlines
                while (bytesRead > 0 && (buffer[bytesRead - 1] == '\r' || buffer[bytesRead - 1] == '\n'))
                {
                    buffer[bytesRead - 1] = '\0';
                    bytesRead--;
                }

                if (bytesRead > 0)
                {
                    debug(("CommandPipe: Received command: %s\n", buffer));
                    EnqueueCommand(buffer);
                }
            }
        }

        // Disconnect the pipe
        DisconnectNamedPipe(hPipe);
    }

    debug(("CommandPipe: Listener thread exiting\n"));
    return 0;
}

/************************************************************************/
/*
 * EnqueueCommand: Add a command to the queue (thread-safe)
 */
static void EnqueueCommand(const char *command)
{
    CommandNode *pNode;

    if (command == NULL || strlen(command) == 0)
        return;

    if (strlen(command) > CMDPIPE_MAX_COMMAND)
    {
        debug(("CommandPipe: Command too long, truncating\n"));
    }

    pNode = (CommandNode *)SafeMalloc(sizeof(CommandNode));
    strncpy(pNode->command, command, CMDPIPE_MAX_COMMAND);
    pNode->command[CMDPIPE_MAX_COMMAND] = '\0';
    pNode->next = NULL;

    EnterCriticalSection(&csCommandQueue);

    if (pCommandTail == NULL)
    {
        pCommandHead = pCommandTail = pNode;
    }
    else
    {
        pCommandTail->next = pNode;
        pCommandTail = pNode;
    }

    LeaveCriticalSection(&csCommandQueue);
}

/************************************************************************/
/*
 * DequeueCommand: Get next command from queue (thread-safe)
 * Returns: Command string (must be freed by caller), or NULL if queue empty
 */
static char* DequeueCommand(void)
{
    CommandNode *pNode;
    char *command = NULL;

    EnterCriticalSection(&csCommandQueue);

    if (pCommandHead != NULL)
    {
        pNode = pCommandHead;
        pCommandHead = pNode->next;

        if (pCommandHead == NULL)
            pCommandTail = NULL;

        command = (char *)SafeMalloc(strlen(pNode->command) + 1);
        strcpy(command, pNode->command);

        SafeFree(pNode);
    }

    LeaveCriticalSection(&csCommandQueue);

    return command;
}

/************************************************************************/
/*
 * ProcessCommand: Execute a command as if user typed it
 */
static void ProcessCommand(const char *command)
{
    if (command == NULL || strlen(command) == 0)
        return;

    // Only process commands when in game state
    if (state != STATE_GAME)
    {
        debug(("CommandPipe: Ignoring command (not in game): %s\n", command));
        return;
    }

    debug(("CommandPipe: Processing command: %s\n", command));

    // Set the text in the input box
    TextInputSetText((char *)command, FALSE);

    // Simulate pressing Enter to send the command
    // We do this by calling PerformAction with A_TEXTCOMMAND
    PerformAction(A_TEXTCOMMAND, NULL);
}

/************************************************************************/
/*
 * CommandPipePoll: Check for pending commands and execute them
 * This should be called from the main game loop
 */
void CommandPipePoll(void)
{
    char *command;

    // Process all pending commands
    while ((command = DequeueCommand()) != NULL)
    {
        ProcessCommand(command);
        SafeFree(command);
    }
}

#ifdef __cplusplus
}
#endif
