#ifndef EPG_SOURCE_H
#define EPG_SOURCE_H

#ifndef __cplusplus
#error This is a C++ include file
#endif


#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include "Reporter.h"
#include "AudioSource.h"
#include "WavSource.h"
#include "OssDspSource.h"
//#include "AlsaDspSource.h"

class EpgSource : public AudioSource, public virtual Reporter
{

    private:
    char dbhost[32];
    char dbname[32];
    char dbuser[32];
    char dbpasswd[32];
    //char dbtable[32];
    //epg_id;

    MYSQL mysql;
    bool mysqlconnected;
    bool isOpened, isdev;
    bool lastFileEnd, started;

    time_t startTime, endTime;
    struct timeval initTime;
    int elapse;
    double elapseLeft;
    double playTime, transBytesTotal, transBytesCur;
    char sourceType[16];
    char fileName[1024];
    char filePath[256];
    char nextFileName[256];
    char deviceID[256];
    char pathcfg[256];
    char blankPath[256];
    char blankFileName[256];
    int minTime;

    int mp3_samples;
    int mp3_bitsPerSample;
    int mp3_channel;
    int isDevice;
    int samples;
    int bitsPerSample;
    int channel;
    int count;

    AudioSource *pSource;


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
            this->samples = sampleRate;
            this->bitsPerSample = bitsPerSample;
            this->channel = channel;
            init(name);
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
            //return pSource != NULL;
	    return isOpened;
        }


	virtual bool
        canRead (     unsigned int    sec,
                                unsigned int    usec )  throw ( Exception );

       virtual unsigned int
        read (                  void          * buf,
                                unsigned int    len )   throw ( Exception );

       virtual void
        close ( void )                                  throw ( Exception );

	 int prepareNextSource(void) ;

	 int setReadRange(int startTime,int endTime=0, int elapse=0) ;

	 time_t genTime(char *strtime, time_t now) ;

     //int filter(const struct dirent*);

     int newPathcfg(FILE*);
     int gets_b_c(char * name);

     int getFileFromPath(void);

     int prepareNextProgram(void);

     void printTime(time_t);

     int check_mysql_connect(MYSQL*);

     void prepareBlankProgram(void);

     void newNextSource(char*, char*);

};

#endif
