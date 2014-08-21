/*------------------------------------------------------------------------------

   Copyright (c) 2000-2007 Tyrell Corporation. All rights reserved.

   Tyrell DarkIce

   File     : main.cpp
   Version  : $Revision: 476 $
   Author   : $Author: rafael@riseup.net $
   Location : $HeadURL$

   Abstract :

    Program entry point

   Copyright notice:

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 3
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

------------------------------------------------------------------------------*/

/* ============================================================ include files */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
#error needs stdlib.h
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#else
#error needs signal.h
#endif

#include <iostream>
#include <fstream>

#include "Ref.h"
#include "Exception.h"
#include "Util.h"
#include "DarkIce.h"
//char *cfgfile = (char *)malloc(256*sizeof(char));
//char *epgTable = (char *)malloc(256*sizeof(char));
//char *freqPath = (char *)malloc(256*sizeof(char));
char *cfgfile, *freqPath, *strSamples, *strBits, *strChannel, *strDevice, *strFileName,*epg_id;


/* ===================================================  local data structures */

/*------------------------------------------------------------------------------
 *  The DarkIce object we're running
 *----------------------------------------------------------------------------*/
static Ref<DarkIce>     darkice;


/* ================================================  local constants & macros */

/*------------------------------------------------------------------------------
 *  File identity
 *----------------------------------------------------------------------------*/
static const char fileid[] = "$Id: main.cpp 476 2010-05-10 01:30:13Z rafael@riseup.net $";

/*------------------------------------------------------------------------------
 *  Default config file name
 *----------------------------------------------------------------------------*/
static const char *DEFAULT_CONFIG_FILE = "/usr/local/etc/radio/darkice.cfg`";


/* ===============================================  local function prototypes */

/*------------------------------------------------------------------------------
 *  Show program usage
 *----------------------------------------------------------------------------*/
static void
showUsage (     std::ostream  & os );

/*------------------------------------------------------------------------------
 *  Handler for the SIGUSR1 signal
 *----------------------------------------------------------------------------*/
static void
sigusr1Handler(int  value);

/* =============================================================  module code */

/*------------------------------------------------------------------------------
 *  Program entry point
 *----------------------------------------------------------------------------*/
int
main (
    int     argc,
    char  * argv[] )
{
    int     res = -1;

    std::cout << "DONGZI " << VERSION
         << " live audio sipsys , make by DongZi "
         << std::endl
         << "Copyright (c) 2000-2007, DongZi, "
         << std::endl
         << "Copyright (c) 2008-2100, DongZi   "
         << std::endl
         << "This is  software, and you are not welcome to redistribute it "
         << std::endl
	 << "under the terms of The GNU General Public License version 3 or"
         << std::endl
	 << "any later version."
         << std::endl << std::endl;

    try {
        const char    * configFileName = DEFAULT_CONFIG_FILE;
        unsigned int    verbosity      = 10;
        int             i;
        //const char      opts[] = "hc:t:p:v:s:b:l:d:f:";
        const char opts[] ="hc:i:p:v:s:b:l:d:f:";

        while ( (i = getopt( argc, argv, opts)) != -1 ) {
            switch ( i ) {
                case 'c':
                    configFileName = optarg;
		            cfgfile = (char *)malloc(256);
		            sprintf(cfgfile,"%s",optarg);
                    break;
                case 'i':
                   //pgTable = (char *)malloc(256);
                    //printf(epgTable, "%s", optarg);
                    epg_id=(char *)malloc(256);
                    sprintf(epg_id,"%s",optarg);
                    Reporter::reportEvent(1,"Now Starting in Fre:",epg_id);
                    //epg_id=Util::strToL(optarg);
                    break;
                case 'p':
                    freqPath = (char *)malloc(256);
                    sprintf(freqPath, "%s", "/Release/live/");
                    break;
                case 's':
                    strSamples = (char *)malloc(10);
                    sprintf(strSamples, "%s", optarg);
                    break;
                case 'b':
                    strBits = (char *)malloc(10);
                    sprintf(strBits, "%s", optarg);
                    break;
                case 'l':
                    strChannel = (char *)malloc(10);
                    sprintf(strChannel, "%s", optarg);
                    break;
                case 'd':
                    strDevice = (char *)malloc(256);
                    sprintf(strDevice, "%s", "epg");
                    //set Device type an epg .when need can change the type
                    //in epg source
                    break;
                case 'f':
                //the name of outputs
                    strFileName = (char *)malloc(256);
                    sprintf(strFileName, "%s", optarg);
                    break;
                case 'v':
                    verbosity = Util::strToL( optarg);
                    break;

                default:
                case ':':
                case '?':
                case 'h':
                    showUsage( std::cout);
                    return 1;
            }
        }
        Reporter::reportEvent( 1, "Using config file:", configFileName);
        std::ifstream       configFile( configFileName);
        Reporter::setReportVerbosity( verbosity );
        Reporter::setReportOutputStream( std::cout );
        Config              config( configFile);

        darkice = new DarkIce( config);

        signal(SIGUSR1, sigusr1Handler);

        res = darkice->run();

    } catch ( Exception   & e ) {
        std::cout << "DongZ: " << e << std::endl << std::flush;
    }

	printf("run ends\n");
	Reporter::reportEvent(1,"Run End ....Please Check The Source That may notReading !!!!!!!! ");
    return res;
}


/*------------------------------------------------------------------------------
 *  Show program usage
 *----------------------------------------------------------------------------*/
static void
showUsage (     std::ostream      & os )
{
    os
    << "usage: darkice [-v n] -c config.file"
    << std::endl
    << std::endl
    << "options:"
    << std::endl
    << "   -c config.file     use configuration file config.file"
    << std::endl
    << "                      if not specified, /etc/darkice.cfg is used"
    << std::endl
    << "   -v n               verbosity level (0 = silent, 10 = loud)"
    << std::endl
    << "   -h                 print this message and exit"
    << std::endl
    << std::endl;
}


/*------------------------------------------------------------------------------
 *  Handle the SIGUSR1 signal here
 *----------------------------------------------------------------------------*/
static void
sigusr1Handler(int    value)
{
    darkice->cut();
}

