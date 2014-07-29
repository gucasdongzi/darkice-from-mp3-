#ifndef EPG_SOURCE_H
#define EPG_SOURCE_H

#ifndef __cplusplus
#error This is a C++ include file
#endif


#include <mysql/mysql.h>
#include <stdio.h>
#include "Reporter.h"
#include "AudioSource.h"
//#include "WavSource.h"
//#include "OssDspSource.h"

class EpgSource : public AudioSource, public virtual Reporter
{
     private:

	 char dbhost[32];
	 char dbname[32];
	 char dbuser[32];
	 char dbpasswd[32];
	 char dbtable[32];

	  MYSQL mysql;
         bool connected;


	 time_t startTime,fileStartTime;
	 time_t endTime,fileEndTime;
	 int elapse;
	 char sourceType[16];
	 char fileName[64];	 

	 int samples;
	 int bitsPerSample;
	 int channel;
	 int sampleRate;
		 
	 AudioSource *pSource;

	 int count;

    protected:

        inline
        EpgSource ( void )                       throw ( Exception )
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
        EpgSource (  const char    * name,
                      int             sampleRate    = 44100,
                      int             bitsPerSample = 16,
                      int             channel       = 2 )
                                                        throw ( Exception )
                    : AudioSource( sampleRate, bitsPerSample, channel)
        {
            init( name);
        }


      inline
        EpgSource (  const EpgSource &    ods )   throw ( Exception )
                    : AudioSource( ods )
        {
            init( ods.fileName);
        }
  	

      inline virtual
        ~EpgSource ( void )                          throw ( Exception )
        {
            strip();
        }

        inline virtual EpgSource &
        operator= (     const EpgSource &     ds )   throw ( Exception )
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


        inline virtual bool
        isOpen ( void ) const      throw ()
        {
            return pSource != NULL;
        }       


	virtual bool
        canRead (     unsigned int    sec,
                                unsigned int    usec )  throw ( Exception );

       virtual unsigned int
        read (                  void          * buf,
                                unsigned int    len )   throw ( Exception );

       virtual void
        close ( void )                                  throw ( Exception );

	 void prepareNextSource(void) ;

	 bool  setReadRange(int startTime,int endTime=0, int elapse=0) ;

	 int genTime(char *strtime) ;

};

#endif
