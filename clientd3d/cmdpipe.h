// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * cmdpipe.h: Named Pipe for external command injection
 *
 * This module implements a Named Pipe server in the client that allows
 * external tools (like admin tools) to inject commands into the client
 * text input as if the user typed them. This replaces unreliable sendkeys.
 */

#ifndef _CMDPIPE_H
#define _CMDPIPE_H

// Named pipe configuration
#define CMDPIPE_NAME "\\\\.\\pipe\\Meridian59_Command_%d"  // %d = process ID
#define CMDPIPE_BUFFER_SIZE 4096
#define CMDPIPE_TIMEOUT 5000

// Maximum command length (same as MAXSAY in client)
#define CMDPIPE_MAX_COMMAND 200

#ifdef __cplusplus
extern "C" {
#endif

// Initialize command pipe system
void CommandPipeInit(void);

// Shutdown command pipe system
void CommandPipeClose(void);

// Check for pending commands and execute them
void CommandPipePoll(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _CMDPIPE_H */
