/*
 * josua - Jack's open sip User Agent
 *
 * Copyright (C) 2002,2003   Aymeric Moizard <jack@atosc.org>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with dpkg; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "gui_setup.h"

gui_t gui_window_setup = {
  GUI_OFF,
  0,
  -999,
  10,
  -6,
  NULL,
  &window_setup_print,
  &window_setup_run_command,
  NULL,
  window_setup_draw_commands,
  -1,
  -1,
  -1,
  NULL
};

int cursor_setup = 0;

typedef struct _j_codec
{
  char payload[10];
  char codec[200];
  int enabled;
} j_codec_t;

j_codec_t j_codec[] = {
  {{"0"}, {"PCMU/8000"}, 0},
  {{"8"}, {"PCMA/8000"}, 0},
  {{"3"}, {"GSM/8000"}, 0},
  {{"110"}, {"speex/8000"}, 0},
  {{"111"}, {"speex/16000"}, 0},
  {{"-1"}, {""}, -1}
};

int
window_setup_print ()
{
  int k;
  int y, x;
  char buf[250];

  josua_clear_box_and_commands (gui_windows[EXTRAGUI]);

  curseson ();
  cbreak ();
  noecho ();
  nonl ();
  keypad (stdscr, TRUE);

  getmaxyx (stdscr, y, x);
  attrset (A_NORMAL);
  attrset (COLOR_PAIR (1));

  if (gui_window_setup.x1 == -999)
    {
  } else
    x = gui_window_setup.x1;

  attrset (COLOR_PAIR (0));

  for (k = 0; j_codec[k].codec[0] != '\0'; k++)
    {
      snprintf (buf, 199, "   %c%c %s %-3.3s %-150.150s ",
                (cursor_setup == k) ? '-' : ' ',
                (cursor_setup == k) ? '>' : ' ',
                (j_codec[k].enabled == 0) ? "ON " : "   ",
                j_codec[k].payload, j_codec[k].codec);

      /* attrset(COLOR_PAIR(0));
         attrset((k==cursor_setup) ? A_REVERSE : A_NORMAL); */
      attrset ((k == cursor_setup) ? A_REVERSE : COLOR_PAIR (1));

      mvaddnstr (gui_window_setup.y0 + 1 + k,
                 gui_window_setup.x0, buf, x - gui_window_setup.x0 - 1);
      if (k > y + gui_window_setup.y1 - gui_window_setup.y0 + 1)
        break;                  /* do not print next one */
    }

  window_setup_draw_commands ();
  return 0;
}

void
window_setup_draw_commands ()
{
  int x, y;
  char *setup_commands[] = {
    "<-", "PrevWindow",
    "->", "NextWindow",
    "e", "Enable",
    "d", "Disable",
    "q", "Move Up",
    "w", "Move Down",
    NULL
  };
  getmaxyx (stdscr, y, x);
  josua_print_command (setup_commands, y - 5, 0);
}

int
window_setup_run_command (int c)
{
  int max;
  int k, y, x;

  curseson ();
  cbreak ();
  noecho ();
  nonl ();
  keypad (stdscr, TRUE);

  getmaxyx (stdscr, y, x);

  if (gui_window_setup.x1 == -999)
    {
  } else
    x = gui_window_setup.x1;

  if (gui_window_setup.y1 < 0)
    max = y + gui_window_setup.y1 - gui_window_setup.y0 + 2;
  else
    max = gui_window_setup.y1 - gui_window_setup.y0 + 2;

  for (k = 0; j_codec[k].codec[0] != '\0'; k++)
    {
    }
  if (k < max)
    max = k;

  k = 0;                        /* codec list untouched */
  switch (c)
    {
      case KEY_DOWN:
        cursor_setup++;
        cursor_setup %= max;
        break;
      case KEY_UP:
        cursor_setup += max - 1;
        cursor_setup %= max;
        break;
      case 'q':
        if (cursor_setup == 0)
          {
            beep ();
            break;
          }
        cursor_setup += max - 1;
        cursor_setup %= max;
        k = 1;
        {
          j_codec_t codec;

          memcpy (&codec, &j_codec[cursor_setup], sizeof (j_codec_t));
          memcpy (&j_codec[cursor_setup], &j_codec[cursor_setup + 1],
                  sizeof (j_codec_t));
          memcpy (&j_codec[cursor_setup + 1], &codec, sizeof (j_codec_t));
        }
        break;
      case 'w':
        if (cursor_setup == max - 1)
          {
            beep ();
            break;
          }
        cursor_setup++;
        cursor_setup %= max;
        k = 1;
        {
          j_codec_t codec;

          memcpy (&codec, &j_codec[cursor_setup], sizeof (j_codec_t));
          memcpy (&j_codec[cursor_setup], &j_codec[cursor_setup - 1],
                  sizeof (j_codec_t));
          memcpy (&j_codec[cursor_setup - 1], &codec, sizeof (j_codec_t));
        }
        break;
      case 'e':
        j_codec[cursor_setup].enabled = 0;
        k = 1;                  /* rebuilt set of codec */
        break;
      case 'd':
        j_codec[cursor_setup].enabled = -1;
        k = 1;                  /* rebuilt set of codec */
        break;
      default:
        beep ();
        return -1;
    }

  if (k == 1)
    {
#if 0
      eXosip_sdp_negotiation_remove_audio_payloads ();
#endif
      for (k = 0; j_codec[k].codec[0] != '\0'; k++)
        {
          char tmp[40];

          if (j_codec[k].enabled == 0)
            {
              snprintf (tmp, 40, "%s %s", j_codec[k].payload, j_codec[k].codec);
#if 0
              eXosip_sdp_negotiation_add_codec (osip_strdup
                                                (j_codec[k].payload), NULL,
                                                osip_strdup ("RTP/AVP"), NULL,
                                                NULL, NULL, NULL, NULL,
                                                osip_strdup (tmp));
#endif
            }
        }
    }

  if (gui_window_setup.on_off == GUI_ON)
    window_setup_print ();
  return 0;
}
