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
 #include <unistd.h>

 #include "audio.h"

 struct ABLS_AGENT *Agent = NULL;                                                                     /* Structure de l'agent */
 struct ABLS_AUDIO_VARS *Agent_vars = NULL;                                             /* Structure des variables de l'agent */

/******************************************************************************************************************************/
/* Play_google_speech: Génère ou lit un message audio à partir d'un libellé                                                   */
/* Entrée: la structure agent et le libellé audio                                                                             */
/* Sortie: aucune                                                                                                             */
/******************************************************************************************************************************/
static void Play_google_speech( const gchar *audio_libelle )
 { gchar safe_name[256];
   gchar filename[512];
   struct stat st;

   if (!audio_libelle || !*audio_libelle) return;

   gchar *language = Agent_config_get_string ( Agent, "language" );
   if (!language || !*language) language = AUDIO_DEFAULT_LANGUAGE;

   Info(__func__, Agent->agent_classe, Agent->agent_tech_id, LOG_NOTICE, "Sending '%s'", audio_libelle);

   g_snprintf(safe_name, sizeof(safe_name), "%s", audio_libelle);
   g_strcanon(safe_name, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz_", '_');
   if (!safe_name[0]) g_snprintf(safe_name, sizeof(safe_name), "speech");

   g_mkdir_with_parents("audio", 0755);
   g_snprintf(filename, sizeof(filename), "audio/%s.mp3", safe_name);

   if (stat(filename, &st) == -1)
    { Info(__func__, Agent->agent_classe, Agent->agent_tech_id, LOG_NOTICE, "Creating file '%s'", filename);
      Run_shell ( "gtts-cli -l %s \"%s\" -o \"%s\"", language, audio_libelle, filename);
    }

   Info(__func__, Agent->agent_classe, Agent->agent_tech_id, LOG_INFO, "Running mpg123 '%s'", filename);
   Run_shell ("mpg123 \"%s\"", filename);

   Agent_send_comm_to_master(Agent, TRUE);
 }
/******************************************************************************************************************************/
/* Audio_set_volume: Configure le volume de sortie audio                                                                      */
/* Entrée: le volume à configurer (0-100)                                                                                     */
/* Sortie: aucune                                                                                                             */
/******************************************************************************************************************************/
static void Audio_set_volume( guint volume )
 { if (volume > 100) volume = 100;

   Run_shell ( "wpctl set-volume @DEFAULT_AUDIO_SINK@ %d%%", volume);
   Info(__func__, Agent->agent_classe, Agent->agent_tech_id, LOG_NOTICE, "Volume set to %d", volume);
 }
/******************************************************************************************************************************/
/* Subscribe_audio_zones: Souscrit l'agent aux zones audio configurées                                                        */
/* Entrée: la structure agent                                                                                                 */
/* Sortie: aucune                                                                                                             */
/******************************************************************************************************************************/
static void Subscribe_audio_zones( JsonArray *audio_zones )
 { if (audio_zones)
    { for (guint i = 0; i < json_array_get_length(audio_zones); i++)
       { JsonNode *element = json_array_get_element(audio_zones, i);
         gchar *audio_zone_name = Json_get_string(element, "audio_zone_name");
         if (audio_zone_name)
          { Info(__func__, Agent->agent_classe, Agent->agent_tech_id, LOG_NOTICE, "Listening to AudioZone '%s'", audio_zone_name);
            Mqtt_subscribe(Agent->mqtt_local, "AUDIO_ZONE/%s", audio_zone_name);
          }
       }
    }
   Info(__func__, Agent->agent_classe, Agent->agent_tech_id, LOG_NOTICE, "Listening to AudioZone 'ALL'");
   Mqtt_subscribe(Agent->mqtt_local, "AUDIO_ZONE/ALL" );                               /* Par défaut, subscribe to zone 'ALL' */
 }
/******************************************************************************************************************************/
/* main: Initialise l'agent audio puis traite la boucle principale                                                            */
/* Entrée: argc et argv                                                                                                       */
/* Sortie: code de retour du programme                                                                                        */
/******************************************************************************************************************************/
gint main(gint argc, gchar *argv[])
 { Config_add_parameter ( "volume",   "0-100",    "Volume de l'agent audio", CONFIG_INT );
   Config_add_parameter ( "language", "fr, en",   "Langue de l'agent audio", CONFIG_STRING );
   Agent = Agent_init(argv[0], "audio", ABLS_AGENT_AUDIO_VERSION, sizeof(struct ABLS_AUDIO_VARS), argc, argv);
   Agent_vars = Agent->vars;

   Audio_set_volume( Agent_config_get_int( Agent, "volume" ) );
   Subscribe_audio_zones( Agent_config_get_array ( Agent, "audio_zones" ) );

   Play_google_speech( "Module audio démarré" );
   Agent_send_comm_to_master(Agent, TRUE);

   Agent_is_ready ( Agent );

   while (Agent->Agent_run == AGENT_IS_RUNNING)
    { Agent_loop(Agent);
/****************************************************** Ecoute du master ******************************************************/
      JsonNode *mqtt_local_message;
      while ( (mqtt_local_message = Agent_get_mqtt_local_message ( Agent ) ) != NULL )
       { if (Mqtt_topic_is(mqtt_local_message, 2, "AUDIO_ZONE", "+") && Json_has_member(mqtt_local_message, "audio_libelle"))
          { gchar *audio_zone_name = Json_get_string(mqtt_local_message, "mqtt_topic_lvl1");
            gchar *audio_libelle = Json_get_string(mqtt_local_message, "audio_libelle");
            time_t now = time(NULL);

            Info( __func__, Agent->agent_classe, Agent->agent_tech_id, LOG_INFO, "Saying '%s' on audio_zone '%s'",
                  audio_libelle, (audio_zone_name ? audio_zone_name : "unknown"));

            if (Agent_vars->last_audio + AUDIO_JINGLE < now)
             { Play_google_speech( "Attention"); }
            Agent_vars->last_audio = now;
            Play_google_speech( audio_libelle );
          }
       }
/****************************************************** Ecoute de l'api *******************************************************/
      JsonNode *mqtt_api_message;
      while ((mqtt_api_message = Agent_get_mqtt_api_message(Agent)) != NULL)
       { if ( Mqtt_topic_is ( mqtt_api_message, 4, "+", "AGENT", Agent->agent_tech_id, "TEST" ) )
          { Info(__func__, Agent->agent_classe, Agent->agent_tech_id, LOG_NOTICE, "Saying 'test'");
            Play_google_speech( "Ceci est un test");
          }
         Json_unref(mqtt_api_message);
       }
    }

   Agent_end(Agent);
 }
/*----------------------------------------------------------------------------------------------------------------------------*/
