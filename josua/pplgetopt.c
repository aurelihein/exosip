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


/*
 * Copyright (c) 1987, 1993, 1994
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <ppl_getopt.h>
#include <osipparser2/osip_port.h>

#if defined(HAVE_STRING_H)
# include <string.h>
#elif defined (HAVE_STRINGS_H)
# include <strings.h>
#else
# include <string.h>
#endif

#define PPL_DECLARE(P) P

#define EMSG    ""

/* Filename_of_pathname returns the final element of the pathname.
 * Using the current platform's filename syntax.
 *   "/foo/bar/gum" -> "gum"
 *   "/foo/bar/gum/" -> ""
 *   "gum" -> "gum"
 *   "wi\\n32\\stuff" -> "stuff
 *
 * Corrected Win32 to accept "a/b\\stuff", "a:stuff"
 */

PPL_DECLARE (const char *)
ppl_filename_of_pathname (const char *pathname)
{
  const char path_separator = '/';
  const char *s = strrchr (pathname, path_separator);

#ifdef WIN32
  const char path_separator_win = '\\';
  const char drive_separator_win = ':';
  const char *s2 = strrchr (pathname, path_separator_win);

  if (s2 > s)
    s = s2;

  if (!s)
    s = strrchr (pathname, drive_separator_win);
#endif

  return s ? ++s : pathname;
}

PPL_DECLARE (ppl_status_t)
ppl_getopt_init (ppl_getopt_t ** os, int argc, const char *const *argv)
{
  void *argv_buff;

  *os = (ppl_getopt_t *) osip_malloc (sizeof (ppl_getopt_t));
  (*os)->reset = 0;
  (*os)->errfn = (ppl_getopt_err_fn_t *) (fprintf);
  (*os)->errarg = (void *) (stderr);

  (*os)->place = EMSG;
  (*os)->argc = argc;

  /* The argv parameter must be compatible with main()'s argv, since
     that's the primary purpose of this function.  But people might
     want to use this function with arrays other than the main argv,
     and we shouldn't touch the caller's data.  So we copy. */
  argv_buff = (char *) osip_malloc ((argc + 1) * sizeof (const char *));
  memcpy (argv_buff, argv, argc * sizeof (const char *));
  (*os)->argv = argv_buff;
  (*os)->argv[argc] = NULL;

  (*os)->interleave = 0;
  (*os)->ind = 1;
  (*os)->skip_start = 1;
  (*os)->skip_end = 1;

  return PPL_SUCCESS;
}

PPL_DECLARE (ppl_status_t)
ppl_getopt (ppl_getopt_t * os, const char *opts, char *optch, const char **optarg)
{
  const char *oli;              /* option letter list index */

  if (os->reset || !*os->place)
    {                           /* update scanning pointer */
      os->reset = 0;
      if (os->ind >= os->argc || *(os->place = os->argv[os->ind]) != '-')
        {
          os->place = EMSG;
          *optch = os->opt;
          return (PPL_EOF);
        }
      if (os->place[1] && *++os->place == '-')
        {                       /* found "--" */
          ++os->ind;
          os->place = EMSG;
          *optch = os->opt;
          return (PPL_EOF);
        }
    }                           /* option letter okay? */
  if ((os->opt = (int) *os->place++) == (int) ':' ||
      !(oli = strchr (opts, os->opt)))
    {
      /*
       * if the user didn't specify '-' as an option,
       * assume it means -1.
       */
      if (os->opt == (int) '-')
        {
          *optch = os->opt;
          return (PPL_EOF);
        }
      if (!*os->place)
        ++os->ind;
      if (os->errfn && *opts != ':')
        {
          (os->errfn) (os->errarg, "%s: illegal option -- %c\n",
                       ppl_filename_of_pathname (*os->argv), os->opt);
        }
      *optch = os->opt;
      return (PPL_BADCH);
    }
  if (*++oli != ':')
    {                           /* don't need argument */
      *optarg = NULL;
      if (!*os->place)
        ++os->ind;
  } else
    {                           /* need an argument */
      if (*os->place)           /* no white space */
        *optarg = os->place;
      else if (os->argc <= ++os->ind)
        {                       /* no arg */
          os->place = EMSG;
          if (*opts == ':')
            {
              *optch = os->opt;
              return (PPL_BADARG);
            }
          if (os->errfn)
            {
              (os->errfn) (os->errarg, "%s: option requires an argument -- %c\n",
                           ppl_filename_of_pathname (*os->argv), os->opt);
            }
          *optch = os->opt;
          return (PPL_BADCH);
      } else                    /* white space */
        *optarg = os->argv[os->ind];
      os->place = EMSG;
      ++os->ind;
    }
  *optch = os->opt;
  return PPL_SUCCESS;
}

/* Reverse the sequence argv[start..start+len-1]. */
static void
reverse (const char **argv, int start, int len)
{
  const char *temp;

  for (; len >= 2; start++, len -= 2)
    {
      temp = argv[start];
      argv[start] = argv[start + len - 1];
      argv[start + len - 1] = temp;
    }
}

/*
 * Permute os->argv with the goal that non-option arguments will all
 * appear at the end.  os->skip_start is where we started skipping
 * non-option arguments, os->skip_end is where we stopped, and os->ind
 * is where we are now.
 */
static void
permute (ppl_getopt_t * os)
{
  int len1 = os->skip_end - os->skip_start;
  int len2 = os->ind - os->skip_end;

  if (os->interleave)
    {
      /*
       * Exchange the sequences argv[os->skip_start..os->skip_end-1] and
       * argv[os->skip_end..os->ind-1].  The easiest way to do that is
       * to reverse the entire range and then reverse the two
       * sub-ranges.
       */
      reverse (os->argv, os->skip_start, len1 + len2);
      reverse (os->argv, os->skip_start, len2);
      reverse (os->argv, os->skip_start + len2, len1);
    }

  /* Reset skip range to the new location of the non-option sequence. */
  os->skip_start += len2;
  os->skip_end += len2;
}

/* Helper function to print out an error involving a long option */
static ppl_status_t
serr (ppl_getopt_t * os, const char *err, const char *str, ppl_status_t status)
{
  if (os->errfn)
    (os->errfn) (os->errarg, "%s: %s: %s\n",
                 ppl_filename_of_pathname (*os->argv), err, str);
  return status;
}

/* Helper function to print out an error involving a short option */
static ppl_status_t
cerr (ppl_getopt_t * os, const char *err, int ch, ppl_status_t status)
{
  if (os->errfn)
    (os->errfn) (os->errarg, "%s: %s: %c\n",
                 ppl_filename_of_pathname (*os->argv), err, ch);
  return status;
}

PPL_DECLARE (ppl_status_t)
ppl_getopt_long (ppl_getopt_t * os,
                 const ppl_getopt_option_t * opts, int *optch, const char **optarg)
{
  const char *p;
  int i;

  /* Let the calling program reset option processing. */
  if (os->reset)
    {
      os->place = EMSG;
      os->ind = 1;
      os->reset = 0;
    }

  /*
   * We can be in one of two states: in the middle of processing a
   * run of short options, or about to process a new argument.
   * Since the second case can lead to the first one, handle that
   * one first.  */
  p = os->place;
  if (*p == '\0')
    {
      /* If we are interleaving, skip non-option arguments. */
      if (os->interleave)
        {
          while (os->ind < os->argc && *os->argv[os->ind] != '-')
            os->ind++;
          os->skip_end = os->ind;
        }
      if (os->ind >= os->argc || *os->argv[os->ind] != '-')
        {
          os->ind = os->skip_start;
          return PPL_EOF;
        }

      p = os->argv[os->ind++] + 1;
      if (*p == '-' && p[1] != '\0')
        {                       /* Long option */
          /* Search for the long option name in the caller's table. */
          int len = 0;

          p++;
          for (i = 0;; i++)
            {
              if (opts[i].optch == 0)   /* No match */
                return serr (os, "invalid option", p - 2, PPL_BADCH);
              len = strlen (opts[i].name);
              if (strncmp (p, opts[i].name, len) == 0
                  && (p[len] == '\0' || p[len] == '='))
                break;
            }
          *optch = opts[i].optch;

          if (opts[i].has_arg)
            {
              if (p[len] == '=')        /* Argument inline */
                *optarg = p + len + 1;
              else if (os->ind >= os->argc)     /* Argument missing */
                return serr (os, "missing argument", p - 2, PPL_BADARG);
              else              /* Argument in next arg */
                *optarg = os->argv[os->ind++];
          } else
            {
              *optarg = NULL;
              if (p[len] == '=')
                return serr (os, "erroneous argument", p - 2, PPL_BADARG);
            }
          permute (os);
          return PPL_SUCCESS;
      } else if (*p == '-')
        {                       /* Bare "--"; we're done */
          permute (os);
          os->ind = os->skip_start;
          return PPL_EOF;
      } else if (*p == '\0')    /* Bare "-" is illegal */
        return serr (os, "invalid option", p, PPL_BADCH);
    }

  /*
   * Now we're in a run of short options, and *p is the next one.
   * Look for it in the caller's table.
   */
  for (i = 0;; i++)
    {
      if (opts[i].optch == 0)   /* No match */
        return cerr (os, "invalid option character", *p, PPL_BADCH);
      if (*p == opts[i].optch)
        break;
    }
  *optch = *p++;

  if (opts[i].has_arg)
    {
      if (*p != '\0')           /* Argument inline */
        *optarg = p;
      else if (os->ind >= os->argc)     /* Argument missing */
        return cerr (os, "missing argument", *optch, PPL_BADARG);
      else                      /* Argument in next arg */
        *optarg = os->argv[os->ind++];
      os->place = EMSG;
  } else
    {
      *optarg = NULL;
      os->place = p;
    }

  permute (os);
  return PPL_SUCCESS;
}
