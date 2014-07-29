/*------------------------------------------------------------------------------

   Copyright (c) 2000-2007 Tyrell Corporation. All rights reserved.
   Copyright (c) 2007 Clyde Stubbs

   Tyrell DarkIce

   File     : WavSource.h
   Version  : $Revision$
   Author   : $Author$
   Location : $HeadURL$
   
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
#ifndef WAV_FILE_SOURCE_H
#define WAV_FILE_SOURCE_H

#ifndef __cplusplus
#error This is a C++ include file
#endif

#include <stdio.h>
#include "Reporter.h"
#include "AudioSource.h"



class WavSource : public AudioSource, public virtual Reporter
{
    private:

       /**
         *  The file name of the wav file .
         */
        char      * fileName;

        /**
         *  The low-level file descriptor .
         */
        FILE*         fileDescriptor;

        /**
         *  Indicates wether the file is in a recording
         *  state.
         */
        bool        running;

        int check_header(FILE *f);		

    protected:


       inline
        WavSource ( void )                       throw ( Exception )
        {
            throw Exception( __FILE__, __LINE__);
        }

	 void
        init (  const char    * name )              throw ( Exception );

        /**
         *  De-iitialize the object
         *
         *  @exception Exception
         */
        void
        strip ( void )                              throw ( Exception );


   public:
   	
	  inline
        WavSource (  const char    * name,
                      int             sampleRate    = 22050,
                      int             bitsPerSample = 16,
                      int             channel       = 1 )
                                                        throw ( Exception )
                    : AudioSource( sampleRate, bitsPerSample, channel)
        {
            init( name);
        }


      inline
        WavSource (  const WavSource &    ods )   throw ( Exception )
                    : AudioSource( ods )
        {
            init( ods.fileName);
        }


       inline virtual
        ~WavSource ( void )                          throw ( Exception )
        {
            strip();
        }

        inline virtual WavSource &
        operator= (     const WavSource &     ds )   throw ( Exception )
        {
            if ( this != &ds ) {
                strip();
                AudioSource::operator=( ds);
                init( ds.fileName);
            }
            return *this;
        }



       virtual bool
        isBigEndian ( void ) const           throw ();



        virtual bool
        open ( void )                                   throw ( Exception );

        /**
         *  Check if the SerialUlaw is open.
         *
         *  @return true if the SerialUlaw is open, false otherwise.
         */
        inline virtual bool
        isOpen ( void ) const      throw ()
        {
            return fileDescriptor != NULL;
        }

        /**
         *  Check if the SerialUlaw can be read from.
         *  Blocks until the specified time for data to be available.
         *  Puts the OSS DSP device into recording mode.
         *
         *  @param sec the maximum seconds to block.
         *  @param usec micro seconds to block after the full seconds.
         *  @return true if the SerialUlaw is ready to be read from,
         *          false otherwise.
         *  @exception Exception
         */
        virtual bool
        canRead (     unsigned int    sec,
                                unsigned int    usec )  throw ( Exception );

        /**
         *  Read from the SerialUlaw.
         *  Puts the OSS DSP device into recording mode.
         *
         *  @param buf the buffer to read into.
         *  @param len the number of bytes to read into buf
         *  @return the number of bytes read (may be less than len).
         *  @exception Exception
         */
        virtual unsigned int
        read (                  void          * buf,
                                unsigned int    len )   throw ( Exception );

        /**
         *  Close the SerialUlaw.
         *
         *  @exception Exception
         */
        virtual void
        close ( void )                                  throw ( Exception );
	   
};

#endif
