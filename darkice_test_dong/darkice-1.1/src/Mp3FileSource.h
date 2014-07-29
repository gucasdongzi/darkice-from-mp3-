/*------------------------------------------------------------------------------

   Copyright (c) 2000-2007 Tyrell Corporation. All rights reserved.
   Copyright (c) 2007 Clyde Stubbs

   Tyrell DarkIce

   File     : Mp3FileSource.h
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
#ifndef MP3_FILE_SOURCE_H
#define MP3_FILE_SOURCE_H

#ifndef __cplusplus
#error This is a C++ include file
#endif


/* ============================================================ include files */

#include "Reporter.h"
#include "AudioSource.h"

#include <mad.h>
#include <sys/wait.h>

# include <semaphore.h>
# include <pthread.h>
#include <sys/mman.h> 
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#else
#error need stdio.h
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
#error need unistd.h
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#else
#error need string.h
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#else
#error need sys/types.h
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#else
#error need sys/stat.h
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#error need fcntl.h
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#error need sys/time.h
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#else
#error need sys/ioctl.h
#endif

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#else
#error need termios.h
#endif


/* ================================================================ constants */


/* =================================================================== macros */


/* =============================================================== data types */

/**
 *  An audio input based on /dev/dsp-like raw devices
 *
 *  @author  $Author: darkeye $
 *  @version $Revision: 1.7 $
 */
class Mp3FileSource: public AudioSource, public virtual Reporter
{
    private:

        /**
         *  The file name of the OSS DSP device (e.g. /dev/dsp or /dev/dsp0).
         */
        char      * fileName;

        /**
         *  The low-level file descriptor of the mp3File.
         */
        int         fileDescriptor;

		struct stat st;
		void *fdm;
		int startOffset;
		int offsetLen;
		int startMSec,endMSec,endOffset;
        int lastPage, curPage, pageSize;
		unsigned int cacheTime;//in S
		unsigned int readOnceTime;//in ms
		unsigned int count;
		unsigned int curReadTime;
		unsigned int buffLen;
		unsigned int buffhead;
		unsigned int bufftail;
		unsigned int outputFlag;
		unsigned int outputLen;
        unsigned int bitRate, pcmBitRate;
		time_t now,initTime;
		double transBytes,playTime;
		sem_t sem_output;
		sem_t sem_read;
		unsigned int endFlag;
		unsigned char ubuff[4608];
		unsigned char *OutputPtr;
		unsigned char Output[4608];
		

        	/**
         	*  Indicates wether the low-level OSS DSP device is in a recording
         	*  state.
         	*/
    		bool    running;
		//bool 	fileEnd;

		//unsigned int frameTime;
		
		pthread_t pthread_1;
    
    		int sid;

    protected:

        /**
         *  Default constructor. Always throws an Exception.
         *
         *  @exception Exception
         */
        inline
        Mp3FileSource( void )                       throw ( Exception )
        {
            throw Exception( __FILE__, __LINE__);
        }

        /**
         *  Initialize the object
         *
         *  @param name the file name of the OSS DSP device.
         *  @exception Exception
         */
        void
        init (  const char    * name )              throw ( Exception );

        /**
         *  De-iitialize the object
         *
         *  @exception Exception
         */


    public:

        /**
         *  Constructor.
         *
         *  @param name the file name of the OSS DSP device
         *              (e.g. /dev/dsp or /dev/dsp0).
         *  @param sampleRate samples per second (e.g. 44100 for 44.1kHz).
         *  @param bitsPerSample bits per sample (e.g. 16 bits).
         *  @param channel number of channels of the audio source
         *                 (e.g. 1 for mono, 2 for stereo, etc.).
         *  @exception Exception
         */
	
        virtual inline void
        strip ( void )					throw ( Exception );
	

        inline
        Mp3FileSource(  const char    * name,
                      int             sampleRate    = 44100,
                      int             bitsPerSample = 16,
                      int             channel       = 2 )
                                                        throw ( Exception )

                    : AudioSource( sampleRate, bitsPerSample, channel)
        {
            pcmBitRate = sampleRate*bitsPerSample*channel;
            init(name);
        }

        /**
         *  Copy Constructor.
         *
         *  @param ods the object to copy.
         *  @exception Exception
         */
        inline
        Mp3FileSource (  const Mp3FileSource&    ods )   throw ( Exception )
                    : AudioSource( ods )
        {
            init( ods.fileName);
        }

        /**
         *  Destructor.
         *
         *  @exception Exception
         */
        inline virtual
        ~Mp3FileSource ( void )                          throw ( Exception )
        {
            strip();
        }

        /**
         *  Assignment operator.
         *
         *  @param ds the object to assign to this one.
         *  @return a reference to this object.
         *  @exception Exception
         */
        inline virtual Mp3FileSource&
        operator= (     const Mp3FileSource&     ds )   throw ( Exception )
        {
            if ( this != &ds ) {
                strip();
                AudioSource::operator=( ds);
                init( ds.fileName);
            }
            return *this;
        }

        /**
         *  Tell if the data from this source comes in big or little endian.
         *
         *  @return true if the source is big endian, false otherwise
         */
        virtual bool
        isBigEndian ( void ) const           throw ();

        /**
         *  Open the SerialUlaw.
         *  This does not put the OSS DSP device into recording mode.
         *  To start getting samples, call either canRead() or read().
         *
         *  @return true if opening was successful, false otherwise
         *  @exception Exception
         *  
         *  @see #canRead
         *  @see #read
         */
        virtual bool
        open ( void )                                   throw ( Exception );

        /**
         *  Check if the SerialUlaw is open.
         *
         *  @return true if the SerialUlaw is open, false otherwise.
         */

        int getFloor(int, int);
        int getCeil(int, int);

		virtual int
		Mp3Locate(void);
		

		int setReadRange(int startTime,int endTime=0,int elapse=0);
		
		
        inline virtual bool
        isOpen ( void ) const                           throw ()
        {
            return fileDescriptor != 0;
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
        canRead (               unsigned int    sec,
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
        
        static void *pthread_decode(void*param);
        //static void *pthread_read();
        int decode( unsigned char const *start, unsigned long length, void *param);
        												
        static enum mad_flow error(	void              *data,
        														struct mad_stream *stream,
        														struct mad_frame  *frame);
        														
        static enum mad_flow output(	void                    *data,
        															struct mad_header const *header,
        															struct mad_pcm 					*pcm);
        															
        static inline signed int scale(mad_fixed_t sample);
        
        static enum mad_flow input(	void 								*data,
        														struct mad_stream 	*stream);
        														
};


/* ================================================= external data structures */


/* ====================================================== function prototypes */



#endif  /* SERIAL_ULAW_SOURCE_H */
