#ifndef _FUNCONFIG_H
#define _FUNCONFIG_H

// Full list of options in ch32fun/ch32fun.h. Defaults: 48 MHz from the HSI.

// No debug printf: it would keep polling the debug module over SWIO.
#define FUNCONF_USE_DEBUGPRINTF 0

#endif
