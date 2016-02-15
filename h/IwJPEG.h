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

#ifndef IW_JPEG_H
#define IW_JPEG_H

// Forward declarations
class CIwTexture;
class CIwImage;

// Decompress JPEG from buffer into texture
void JPEGTexture(void * buf, int len, CIwTexture & ImageTex);
void JPEGImage(void * buf, int len, CIwImage & pImage);

#endif

