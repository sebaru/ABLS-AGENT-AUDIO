/******************************************************************************************************************************/
/* ABLS-AGENT-AUDIO/audio.c  Standalone audio agent                                                                           */
/* Projet Abls-Habitat                   Gestion d'habitat                                                20.07.2026 14:20:00 */
/* Auteur: LEFEVRE Sebastien                                                                                                  */
/******************************************************************************************************************************/
/*
 * audio.c
 * This file is part of Abls-Habitat
 *
 * Copyright (C) 1988-2026 - Sebastien LEFEVRE
 *
 * ABLS-AGENT-AUDIO is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"

/******************************************************************************************************************************/
/* Play_google_speech: Génère ou lit un message audio à partir d'un libellé                                                   */
/* Entrée: la structure agent et le libellé audio                                                                             */
/* Sortie: aucune                                                                                                             */
/******************************************************************************************************************************/
static void Play_google_speech(struct ABLS_AGENT *agent, const gchar *audio_libelle)
 { gchar command[1024];
   gchar safe_name[256];
   gchar filename[512];
   gchar *language;
   struct stat st;

   if (!audio_libelle || !*audio_libelle) return;

   language = Json_get_string(agent->api_config, "language");
   if (!language || !*language) language = AUDIO_DEFAULT_LANGUAGE;

   Info(__func__, agent->agent_classe, agent->agent_tech_id, LOG_NOTICE, "Sending '%s'", audio_libelle);

   g_snprintf(safe_name, sizeof(safe_name), "%s", audio_libelle);
   g_strcanon(safe_name, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz_", '_');
   if (!safe_name[0]) g_snprintf(safe_name, sizeof(safe_name), "speech");

   g_mkdir_with_parents("audio", 0755);
   g_snprintf(filename, sizeof(filename), "audio/%s.mp3", safe_name);

   if (stat(filename, &st) == -1)
    { Info(__func__, agent->agent_classe, agent->agent_tech_id, LOG_NOTICE, "Creating file '%s'", filename);
      g_snprintf(command, sizeof(command), "gtts-cli -l %s \"%s\" -o \"%s\"", language, audio_libelle, filename);
      system(command);
    }

   Info(__func__, agent->agent_classe, agent->agent_tech_id, LOG_INFO, "Running mpg123 '%s'", filename);
   g_snprintf(command, sizeof(command), "mpg123 \"%s\"", filename);
   system(command);

   Agent_send_comm_to_master(agent, TRUE);
 }

/******************************************************************************************************************************/
/* Configure_volume: Configure le volume de sortie audio                                                                      */
/* Entrée: la structure agent                                                                                                 */
/* Sortie: aucune                                                                                                             */
/******************************************************************************************************************************/
static void Configure_volume(struct ABLS_AGENT *agent)
 { gchar command[128];
   gint volume = Json_get_int(agent->api_config, "volume");

   if (volume < 0 || volume > 100) volume = 100;

   g_snprintf(command, sizeof(command), "wpctl set-volume @DEFAULT_AUDIO_SINK@ %d%%", volume);
   system(command);

   Info(__func__, agent->agent_classe, agent->agent_tech_id, LOG_NOTICE, "Volume set to %d", volume);
 }

/******************************************************************************************************************************/
/* Subscribe_audio_zones: Souscrit l'agent aux zones audio configurées                                                        */
/* Entrée: la structure agent                                                                                                 */
/* Sortie: aucune                                                                                                             */
/******************************************************************************************************************************/
static void Subscribe_audio_zones(struct ABLS_AGENT *agent)
 { JsonArray *audio_zones = Json_get_array(agent->api_config, "audio_zones");

   if (!audio_zones) return;

   for (guint i = 0; i < json_array_get_length(audio_zones); i++)
    { JsonNode *element = json_array_get_element(audio_zones, i);
      gchar *audio_zone_name = Json_get_string(element, "audio_zone_name");
      if (audio_zone_name)
       { Info(__func__, agent->agent_classe, agent->agent_tech_id, LOG_NOTICE, "Listening to AudioZone '%s'", audio_zone_name);
         Mqtt_subscribe(agent->mqtt_api, "AUDIO_ZONE/%s", audio_zone_name);
       }
    }
 }
/******************************************************************************************************************************/
/* main: Initialise l'agent audio puis traite la boucle principale                                                            */
/* Entrée: argc et argv                                                                                                       */
/* Sortie: code de retour du programme                                                                                        */
/******************************************************************************************************************************/
gint main(gint argc, gchar *argv[])
 { struct ABLS_AGENT *agent = Agent_init(argv[0], "audio", ABLS_AGENT_AUDIO_VERSION, sizeof(struct ABLS_AUDIO_VARS), argc, argv);
   struct ABLS_AUDIO_VARS *vars = agent->vars;

   Configure_volume(agent);
   Agent_send_comm_to_master(agent, TRUE);
   Subscribe_audio_zones(agent);

   Play_google_speech(agent, "Module audio demarre");

   while (agent->Agent_run == AGENT_IS_RUNNING)
    { Agent_loop(agent);
/****************************************************** Ecoute du master ******************************************************/
      JsonNode *mqtt_local_message;
      while ( (mqtt_local_message = Agent_get_mqtt_local_message ( agent ) ) != NULL )
       { if (Mqtt_topic_is(mqtt_local_message, 2, "AUDIO_ZONE", "+") && Json_has_member(mqtt_local_message, "audio_libelle"))
          { gchar *audio_zone_name = Json_get_string(mqtt_local_message, "mqtt_topic_lvl1");
            gchar *audio_libelle = Json_get_string(mqtt_local_message, "audio_libelle");
            time_t now = time(NULL);

            Info(__func__, agent->agent_classe, agent->agent_tech_id, LOG_INFO, "Saying '%s' on audio_zone '%s'",
                   audio_libelle, (audio_zone_name ? audio_zone_name : "unknown"));

            if (vars->last_audio + AUDIO_JINGLE < now)
             { Play_google_speech(agent, "Attention"); }
            vars->last_audio = now;
            Play_google_speech(agent, audio_libelle);
          }
       }
/****************************************************** Ecoute de l'api *******************************************************/
      JsonNode *mqtt_api_message;
      while ((mqtt_api_message = Agent_get_mqtt_api_message(agent)) != NULL)
       { if ( Mqtt_topic_is ( mqtt_api_message, 4, "+", "AGENT", agent->tech_id, "TEST" ) )
          { Info(__func__, agent->agent_classe, agent->agent_tech_id, LOG_NOTICE, "Saying 'test'");
            Play_google_speech(agent, "Ceci est un test");
          }
         else if (Mqtt_topic_is(mqtt_api_message, 2, "AUDIO_ZONE", "+") && Json_has_member(mqtt_api_message, "audio_libelle"))
          { gchar *audio_zone_name = Json_get_string(mqtt_api_message, "mqtt_topic_lvl1");
            gchar *audio_libelle = Json_get_string(mqtt_api_message, "audio_libelle");
            time_t now = time(NULL);

            Info(__func__, agent->agent_classe, agent->agent_tech_id, LOG_INFO, "Saying '%s' on audio_zone '%s'",
                 audio_libelle, (audio_zone_name ? audio_zone_name : "unknown"));

            if (vars->last_audio + AUDIO_JINGLE < now)
             { Play_google_speech(agent, "Attention"); }
            vars->last_audio = now;
            Play_google_speech(agent, audio_libelle);
          }
         Json_unref(mqtt_api_message);
       }
    }

   Agent_end(agent);
 }
/*----------------------------------------------------------------------------------------------------------------------------*/
