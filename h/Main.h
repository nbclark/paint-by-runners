/*
 * This file is part of the Airplay SDK Code Samples.
 *
 * Copyright (C) 2001-2009 Ideaworks Labs.
 * All Rights Reserved.
 *
 * This source code is intended only as a supplement to Ideaworks Labs
 * Development Tools and/or on-line documentation.
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 */
// Examples main header
//-----------------------------------------------------------------------------

#ifndef EXAMPLES_MAIN_H
#define EXAMPLES_MAIN_H
#include "IGameEngine.h"

// Attempt to lock to 20 frames per second
#define FRAMES_PER_S 30
#define	MS_PER_FRAME (1000 / FRAMES_PER_S)
#define RUNAWAYGAME_ID	2

/*
#define RUNAWAYGAME_SERVER	"localhost"
#define RUNAWAYGAME_PORT	81
*/
#define RUNAWAYGAME_SERVER	"mobilesrc.servegame.com"
#define RUNAWAYGAME_PORT	80

int GameMain();

#endif /* !EXAMPLES_MAIN_H */
