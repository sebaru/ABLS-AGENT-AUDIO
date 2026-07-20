/******************************************************************************************************************************/
/* ABLS-AGENT-AUDIO/include/audio.h   Header and constants for audio agent                                                   */
/* Projet Abls-Habitat                   Gestion d'habitat                                                20.07.2026 14:20:00 */
/* Auteur: LEFEVRE Sebastien                                                                                                  */
/******************************************************************************************************************************/
/*
 * audio.h
 * This file is part of Abls-Habitat
 *
 * Copyright (C) 1988-2026 - Sebastien LEFEVRE
 *
 * ABLS-AGENT-AUDIO is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _ABLS_AUDIO_H_
#define _ABLS_AUDIO_H_

#include <time.h>
#include <abls-agent-libs/abls-agent-libs.h>

#define AUDIO_JINGLE 3000
#define AUDIO_DEFAULT_LANGUAGE "fr"

struct ABLS_AUDIO_VARS
 { time_t last_audio;
 };

#endif
/*----------------------------------------------------------------------------------------------------------------------------*/
