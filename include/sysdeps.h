//
// C++ Interface: sysdeps
//
// Description: System Dependency "equalization"
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SYSDEPS_H
#define SYSDEPS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
#pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdint.h>

#ifndef __MINGW32__
inline int64_t llabs(int64_t x) throw () { return (x < 0) ? -x : x; }
#endif

#ifdef __MINGW32__
inline int ffs(int i) { return __builtin_ffs(i); }
inline int ffsl(long int i) { return __builtin_ffsl(i); }
inline int ffsll(long long int i) { return __builtin_ffsll(i); }

inline char *ctime_r(const time_t *timep, char *buf)
{
	char *cp = ctime(timep);
	if (cp)
		cp = strcpy(buf, cp);
	return cp;
}

inline struct tm *gmtime_r(const time_t *timep, struct tm *result)
{
        struct tm *r1(gmtime(timep));
        if (r1) {
                *result = *r1;
                return result;
        }
        return 0;
}

inline struct tm *localtime_r(const time_t *timep, struct tm *result)
{
        struct tm *r1(localtime(timep));
        if (r1) {
                *result = *r1;
                return result;
        }
        return 0;
}

inline time_t timegm(struct tm *tm)
{
        time_t answer;
        char *zone = getenv("TZ");
        putenv("TZ=UTC");
        tzset();
        answer = mktime(tm);
        if (zone) {
                char *old_zone = (char *)malloc(3+strlen(zone)+1);
                if (old_zone) {
                        strcpy(old_zone, "TZ=");
                        strcat(old_zone, zone);
                        putenv(old_zone);	
                }
        } else {
                putenv("TZ");
        }
        tzset();
        return answer;
}

#endif

#ifdef HAVE_SYSEXITS_H
#include <sysexits.h>
#else
#define EX_USAGE       64
#define EX_DATAERR     65
#define EX_UNAVAILABLE 69
#define EX_OK          0
#endif

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(x) gettext(x)
#else
#define _(x) x
#endif

#ifdef HAVE_GTKMM2
#define UIEXT ".glade"
#endif

#ifdef HAVE_GTKMM3
#define UIEXT ".ui"
#endif

#ifdef HAVE_GDKKEYCOMPAT
#define GDK_KEY_0 GDK_0
#define GDK_KEY_1 GDK_1
#define GDK_KEY_2 GDK_2
#define GDK_KEY_3 GDK_3
#define GDK_KEY_4 GDK_4
#define GDK_KEY_5 GDK_5
#define GDK_KEY_6 GDK_6
#define GDK_KEY_7 GDK_7
#define GDK_KEY_8 GDK_8
#define GDK_KEY_9 GDK_9
#define GDK_KEY_a GDK_a
#define GDK_KEY_A GDK_A
#define GDK_KEY_ampersand GDK_ampersand
#define GDK_KEY_asciicircum GDK_asciicircum
#define GDK_KEY_asciitilde GDK_asciitilde
#define GDK_KEY_asterisk GDK_asterisk
#define GDK_KEY_at GDK_at
#define GDK_KEY_b GDK_b
#define GDK_KEY_B GDK_B
#define GDK_KEY_backslash GDK_backslash
#define GDK_KEY_BackSpace GDK_BackSpace
#define GDK_KEY_bar GDK_bar
#define GDK_KEY_braceleft GDK_braceleft
#define GDK_KEY_braceright GDK_braceright
#define GDK_KEY_bracketleft GDK_bracketleft
#define GDK_KEY_bracketright GDK_bracketright
#define GDK_KEY_c GDK_c
#define GDK_KEY_C GDK_C
#define GDK_KEY_colon GDK_colon
#define GDK_KEY_comma GDK_comma
#define GDK_KEY_d GDK_d
#define GDK_KEY_D GDK_D
#define GDK_KEY_dollar GDK_dollar
#define GDK_KEY_Down GDK_Down
#define GDK_KEY_e GDK_e
#define GDK_KEY_E GDK_E
#define GDK_KEY_equal GDK_equal
#define GDK_KEY_Escape GDK_Escape
#define GDK_KEY_exclam GDK_exclam
#define GDK_KEY_f GDK_f
#define GDK_KEY_F GDK_F
#define GDK_KEY_F6 GDK_F6
#define GDK_KEY_F7 GDK_F7
#define GDK_KEY_F8 GDK_F8
#define GDK_KEY_g GDK_g
#define GDK_KEY_G GDK_G
#define GDK_KEY_greater GDK_greater
#define GDK_KEY_h GDK_h
#define GDK_KEY_H GDK_H
#define GDK_KEY_i GDK_i
#define GDK_KEY_I GDK_I
#define GDK_KEY_j GDK_j
#define GDK_KEY_J GDK_J
#define GDK_KEY_k GDK_k
#define GDK_KEY_K GDK_K
#define GDK_KEY_KP_0 GDK_KP_0
#define GDK_KEY_KP_1 GDK_KP_1
#define GDK_KEY_KP_2 GDK_KP_2
#define GDK_KEY_KP_3 GDK_KP_3
#define GDK_KEY_KP_4 GDK_KP_4
#define GDK_KEY_KP_5 GDK_KP_5
#define GDK_KEY_KP_6 GDK_KP_6
#define GDK_KEY_KP_7 GDK_KP_7
#define GDK_KEY_KP_8 GDK_KP_8
#define GDK_KEY_KP_9 GDK_KP_9
#define GDK_KEY_KP_Add GDK_KP_Add
#define GDK_KEY_KP_Decimal GDK_KP_Decimal
#define GDK_KEY_KP_Divide GDK_KP_Divide
#define GDK_KEY_KP_Enter GDK_KP_Enter
#define GDK_KEY_KP_Multiply GDK_KP_Multiply
#define GDK_KEY_KP_Subtract GDK_KP_Subtract
#define GDK_KEY_l GDK_l
#define GDK_KEY_L GDK_L
#define GDK_KEY_Left GDK_Left
#define GDK_KEY_less GDK_less
#define GDK_KEY_m GDK_m
#define GDK_KEY_M GDK_M
#define GDK_KEY_minus GDK_minus
#define GDK_KEY_n GDK_n
#define GDK_KEY_N GDK_N
#define GDK_KEY_numbersign GDK_numbersign
#define GDK_KEY_o GDK_o
#define GDK_KEY_O GDK_O
#define GDK_KEY_p GDK_p
#define GDK_KEY_P GDK_P
#define GDK_KEY_parenleft GDK_parenleft
#define GDK_KEY_parenright GDK_parenright
#define GDK_KEY_percent GDK_percent
#define GDK_KEY_period GDK_period
#define GDK_KEY_plus GDK_plus
#define GDK_KEY_q GDK_q
#define GDK_KEY_Q GDK_Q
#define GDK_KEY_question GDK_question
#define GDK_KEY_quotedbl GDK_quotedbl
#define GDK_KEY_quoteleft GDK_quoteleft
#define GDK_KEY_quoteright GDK_quoteright
#define GDK_KEY_r GDK_r
#define GDK_KEY_R GDK_R
#define GDK_KEY_Return GDK_Return
#define GDK_KEY_Right GDK_Right
#define GDK_KEY_s GDK_s
#define GDK_KEY_S GDK_S
#define GDK_KEY_semicolon GDK_semicolon
#define GDK_KEY_slash GDK_slash
#define GDK_KEY_space GDK_space
#define GDK_KEY_t GDK_t
#define GDK_KEY_T GDK_T
#define GDK_KEY_Tab GDK_Tab
#define GDK_KEY_u GDK_u
#define GDK_KEY_U GDK_U
#define GDK_KEY_underscore GDK_underscore
#define GDK_KEY_Up GDK_Up
#define GDK_KEY_v GDK_v
#define GDK_KEY_V GDK_V
#define GDK_KEY_w GDK_w
#define GDK_KEY_W GDK_W
#define GDK_KEY_x GDK_x
#define GDK_KEY_X GDK_X
#define GDK_KEY_y GDK_y
#define GDK_KEY_Y GDK_Y
#define GDK_KEY_z GDK_z
#define GDK_KEY_Z GDK_Z
#endif

#endif /* SYSDEPS_H */
