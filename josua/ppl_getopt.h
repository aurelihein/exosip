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

/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#ifndef _PPL_GETOPT_H_
#define _PPL_GETOPT_H_

/* #include "ppl.h" */

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

typedef int ppl_status_t;

#define PPL_SUCCESS   0
#define PPL_START_ERRNO    200
#define PPL_START_STATUS   250

#define PPL_ENOSTAT        (PPL_START_ERRNO + 1)
#define PPL_ENOPOOL        (PPL_START_ERRNO + 2)
/* empty slot: +3 */
/** @see PPL_STATUS_IS_EBADDATE */
#define PPL_EBADDATE       (PPL_START_ERRNO + 4)
/** @see PPL_STATUS_IS_EINVALSOCK */
#define PPL_EINVALSOCK     (PPL_START_ERRNO + 5)
/** @see PPL_STATUS_IS_ENOPROC */
#define PPL_ENOPROC        (PPL_START_ERRNO + 6)
/** @see PPL_STATUS_IS_ENOTIME */
#define PPL_ENOTIME        (PPL_START_ERRNO + 7)
/** @see PPL_STATUS_IS_ENODIR */
#define PPL_ENODIR         (PPL_START_ERRNO + 8)
/** @see PPL_STATUS_IS_ENOLOCK */
#define PPL_ENOLOCK        (PPL_START_ERRNO + 9)
/** @see PPL_STATUS_IS_ENOPOLL */
#define PPL_ENOPOLL        (PPL_START_ERRNO + 10)
/** @see PPL_STATUS_IS_ENOSOCKET */
#define PPL_ENOSOCKET      (PPL_START_ERRNO + 11)
/** @see PPL_STATUS_IS_ENOTHREAD */
#define PPL_ENOTHREAD      (PPL_START_ERRNO + 12)
/** @see PPL_STATUS_IS_ENOTHDKEY */
#define PPL_ENOTHDKEY      (PPL_START_ERRNO + 13)
/** @see PPL_STATUS_IS_EGENERAL */
#define PPL_EGENERAL       (PPL_START_ERRNO + 14)
/** @see PPL_STATUS_IS_ENOSHMAVAIL */
#define PPL_ENOSHMAVAIL    (PPL_START_ERRNO + 15)
/** @see PPL_STATUS_IS_EBADIP */
#define PPL_EBADIP         (PPL_START_ERRNO + 16)
/** @see PPL_STATUS_IS_EBADMASK */
#define PPL_EBADMASK       (PPL_START_ERRNO + 17)
/* empty slot: +18 */
/** @see PPL_STATUS_IS_EDSOPEN */
#define PPL_EDSOOPEN       (PPL_START_ERRNO + 19)
/** @see PPL_STATUS_IS_EABSOLUTE */
#define PPL_EABSOLUTE      (PPL_START_ERRNO + 20)
/** @see PPL_STATUS_IS_ERELATIVE */
#define PPL_ERELATIVE      (PPL_START_ERRNO + 21)
/** @see PPL_STATUS_IS_EINCOMPLETE */
#define PPL_EINCOMPLETE    (PPL_START_ERRNO + 22)
/** @see PPL_STATUS_IS_EABOVEROOT */
#define PPL_EABOVEROOT     (PPL_START_ERRNO + 23)
/** @see PPL_STATUS_IS_EBADPATH */
#define PPL_EBADPATH       (PPL_START_ERRNO + 24)


/* PPL ERROR VALUE TESTS */
/** 
 * PPL was unable to perform a stat on the file 
 * @warning always use this test, as platform-specific variances may meet this
 * more than one error code 
 */
#define PPL_STATUS_IS_ENOSTAT(s)        ((s) == PPL_ENOSTAT)
/** 
 * PPL was not provided a pool with which to allocate memory 
 * @warning always use this test, as platform-specific variances may meet this
 * more than one error code 
 */
#define PPL_STATUS_IS_ENOPOOL(s)        ((s) == PPL_ENOPOOL)
/** PPL was given an invalid date  */
#define PPL_STATUS_IS_EBADDATE(s)       ((s) == PPL_EBADDATE)
/** PPL was given an invalid socket */
#define PPL_STATUS_IS_EINVALSOCK(s)     ((s) == PPL_EINVALSOCK)
/** PPL was not given a process structure */
#define PPL_STATUS_IS_ENOPROC(s)        ((s) == PPL_ENOPROC)
/** PPL was not given a time structure */
#define PPL_STATUS_IS_ENOTIME(s)        ((s) == PPL_ENOTIME)
/** PPL was not given a directory structure */
#define PPL_STATUS_IS_ENODIR(s)         ((s) == PPL_ENODIR)
/** PPL was not given a lock structure */
#define PPL_STATUS_IS_ENOLOCK(s)        ((s) == PPL_ENOLOCK)
/** PPL was not given a poll structure */
#define PPL_STATUS_IS_ENOPOLL(s)        ((s) == PPL_ENOPOLL)
/** PPL was not given a socket */
#define PPL_STATUS_IS_ENOSOCKET(s)      ((s) == PPL_ENOSOCKET)
/** PPL was not given a thread structure */
#define PPL_STATUS_IS_ENOTHREAD(s)      ((s) == PPL_ENOTHREAD)
/** PPL was not given a thread key structure */
#define PPL_STATUS_IS_ENOTHDKEY(s)      ((s) == PPL_ENOTHDKEY)
/** Generic Error which can not be put into another spot */
#define PPL_STATUS_IS_EGENERAL(s)       ((s) == PPL_EGENERAL)
/** There is no more shared memory available */
#define PPL_STATUS_IS_ENOSHMAVAIL(s)    ((s) == PPL_ENOSHMAVAIL)
/** The specified IP address is invalid */
#define PPL_STATUS_IS_EBADIP(s)         ((s) == PPL_EBADIP)
/** The specified netmask is invalid */
#define PPL_STATUS_IS_EBADMASK(s)       ((s) == PPL_EBADMASK)
/* empty slot: +18 */
/** 
 * PPL was unable to open the dso object.  
 * For more information call apr_dso_error().
 */
#define PPL_STATUS_IS_EDSOOPEN(s)       ((s) == PPL_EDSOOPEN)
/** The given path was absolute. */
#define PPL_STATUS_IS_EABSOLUTE(s)      ((s) == PPL_EABSOLUTE)
/** The given path was relative. */
#define PPL_STATUS_IS_ERELATIVE(s)      ((s) == PPL_ERELATIVE)
/** The given path was neither relative nor absolute. */
#define PPL_STATUS_IS_EINCOMPLETE(s)    ((s) == PPL_EINCOMPLETE)
/** The given path was above the root path. */
#define PPL_STATUS_IS_EABOVEROOT(s)     ((s) == PPL_EABOVEROOT)
/** The given path was bad. */
#define PPL_STATUS_IS_EBADPATH(s)       ((s) == PPL_EBADPATH)

/* PPL STATUS VALUES */
/** @see PPL_STATUS_IS_INCHILD */
#define PPL_INCHILD        (PPL_START_STATUS + 1)
/** @see PPL_STATUS_IS_INPARENT */
#define PPL_INPARENT       (PPL_START_STATUS + 2)
/** @see PPL_STATUS_IS_DETACH */
#define PPL_DETACH         (PPL_START_STATUS + 3)
/** @see PPL_STATUS_IS_NOTDETACH */
#define PPL_NOTDETACH      (PPL_START_STATUS + 4)
/** @see PPL_STATUS_IS_CHILD_DONE */
#define PPL_CHILD_DONE     (PPL_START_STATUS + 5)
/** @see PPL_STATUS_IS_CHILD_NOTDONE */
#define PPL_CHILD_NOTDONE  (PPL_START_STATUS + 6)
/** @see PPL_STATUS_IS_TIMEUP */
#define PPL_TIMEUP         (PPL_START_STATUS + 7)
/** @see PPL_STATUS_IS_INCOMPLETE */
#define PPL_INCOMPLETE     (PPL_START_STATUS + 8)
/* empty slot: +9 */
/* empty slot: +10 */
/* empty slot: +11 */
/** @see PPL_STATUS_IS_BADCH */
#define PPL_BADCH          (PPL_START_STATUS + 12)
/** @see PPL_STATUS_IS_BADARG */
#define PPL_BADARG         (PPL_START_STATUS + 13)
/** @see PPL_STATUS_IS_EOF */
#define PPL_EOF            (PPL_START_STATUS + 14)
/** @see PPL_STATUS_IS_NOTFOUND */
#define PPL_NOTFOUND       (PPL_START_STATUS + 15)
/* empty slot: +16 */
/* empty slot: +17 */
/* empty slot: +18 */
/** @see PPL_STATUS_IS_ANONYMOUS */
#define PPL_ANONYMOUS      (PPL_START_STATUS + 19)
/** @see PPL_STATUS_IS_FILEBASED */
#define PPL_FILEBASED      (PPL_START_STATUS + 20)
/** @see PPL_STATUS_IS_KEYBASED */
#define PPL_KEYBASED       (PPL_START_STATUS + 21)
/** @see PPL_STATUS_IS_EINIT */
#define PPL_EINIT          (PPL_START_STATUS + 22)
/** @see PPL_STATUS_IS_ENOTIMPL */
#define PPL_ENOTIMPL       (PPL_START_STATUS + 23)
/** @see PPL_STATUS_IS_EMISMATCH */
#define PPL_EMISMATCH      (PPL_START_STATUS + 24)
/** @see PPL_STATUS_IS_EBUSY */
#define PPL_EBUSY          (PPL_START_STATUS + 25)

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/**
 * @file ppl_getopt.h
 * @brief PPL Command Arguments (getopt)
 */
/**
 * @defgroup PPL_getopt Command Argument Parsing
 * @ingroup PPL
 * @{
 */

  typedef void (ppl_getopt_err_fn_t) (void *arg, const char *err, ...);

  typedef struct ppl_getopt_t ppl_getopt_t;
/**
 * Structure to store command line argument information.
 */
  struct ppl_getopt_t
  {
    /** function to print error message (NULL == no messages) */
    ppl_getopt_err_fn_t *errfn;
    /** user defined first arg to pass to error message  */
    void *errarg;
    /** index into parent argv vector */
    int ind;
    /** character checked for validity */
    int opt;
    /** reset getopt */
    int reset;
    /** count of arguments */
    int argc;
    /** array of pointers to arguments */
    const char **argv;
    /** argument associated with option */
    char const *place;
    /** set to nonzero to support interleaving options with regular args */
    int interleave;
    /** start of non-option arguments skipped for interleaving */
    int skip_start;
    /** end of non-option arguments skipped for interleaving */
    int skip_end;
  };

  typedef struct ppl_getopt_option_t ppl_getopt_option_t;

/**
 * Structure used to describe options that getopt should search for.
 */
  struct ppl_getopt_option_t
  {
    /** long option name, or NULL if option has no long name */
    const char *name;
    /** option letter, or a value greater than 255 if option has no letter */
    int optch;
    /** nonzero if option takes an argument */
    int has_arg;
    /** a description of the option */
    const char *description;
  };

/** Filename_of_pathname returns the final element of the pathname.
 * Using the current platform's filename syntax.
 *   "/foo/bar/gum" -> "gum"
 *   "/foo/bar/gum/" -> ""
 *   "gum" -> "gum"
 *   "wi\\n32\\stuff" -> "stuff
 *
 * Corrected Win32 to accept "a/b\\stuff", "a:stuff"
 */
const char * ppl_filename_of_pathname (const char *pathname);

/**
 * Initialize the arguments for parsing by ppl_getopt().
 * @param os   The options structure created for ppl_getopt()
 * @param cont The pool to operate on
 * @param argc The number of arguments to parse
 * @param argv The array of arguments to parse
 * @remark Arguments 2 and 3 are most commonly argc and argv from main(argc, argv)
 * The errfn is initialized to fprintf(stderr... but may be overridden.
 */
  ppl_status_t ppl_getopt_init (ppl_getopt_t ** os,
				int argc, const char *const *argv);

/**
 * Parse the options initialized by ppl_getopt_init().
 * @param os     The ppl_opt_t structure returned by ppl_getopt_init()
 * @param opts   A string of characters that are acceptable options to the 
 *               program.  Characters followed by ":" are required to have an 
 *               option associated
 * @param option_ch  The next option character parsed
 * @param option_arg The argument following the option character:
 * @return There are four potential status values on exit. They are:
 * <PRE>
 *             PPL_EOF      --  No more options to parse
 *             PPL_BADCH    --  Found a bad option character
 *             PPL_BADARG   --  No argument followed @parameter:
 *             PPL_SUCCESS  --  The next option was found.
 * </PRE>
 */
  ppl_status_t ppl_getopt (ppl_getopt_t * os, const char *opts,
                                         char *option_ch, const char **option_arg);

/**
 * Parse the options initialized by ppl_getopt_init(), accepting long
 * options beginning with "--" in addition to single-character
 * options beginning with "-".
 * @param os     The ppl_getopt_t structure created by ppl_getopt_init()
 * @param opts   A pointer to a list of ppl_getopt_option_t structures, which
 *               can be initialized with { "name", optch, has_args }.  has_args
 *               is nonzero if the option requires an argument.  A structure
 *               with an optch value of 0 terminates the list.
 * @param option_ch  Receives the value of "optch" from the ppl_getopt_option_t
 *                   structure corresponding to the next option matched.
 * @param option_arg Receives the argument following the option, if any.
 * @return There are four potential status values on exit.   They are:
 * <PRE>
 *             PPL_EOF      --  No more options to parse
 *             PPL_BADCH    --  Found a bad option character
 *             PPL_BADARG   --  No argument followed @parameter:
 *             PPL_SUCCESS  --  The next option was found.
 * </PRE>
 * When PPL_SUCCESS is returned, os->ind gives the index of the first
 * non-option argument.  On error, a message will be printed to stdout unless
 * os->err is set to 0.  If os->interleave is set to nonzero, options can come
 * after arguments, and os->argv will be permuted to leave non-option arguments
 * at the end (the original argv is unaffected).
 */
  ppl_status_t ppl_getopt_long (ppl_getopt_t * os,
                                              const ppl_getopt_option_t * opts,
                                              int *option_ch,
                                              const char **option_arg);
/** @} */

#ifdef __cplusplus
}
#endif

#endif                          /* ! PPL_GETOPT_H */
