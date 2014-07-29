#ifndef CIRCLE_SOURCE_H
#define CIRCLE_SOURCE_H

#ifndef __cplusplus
#error This is a C++ include file
#endif


#include <stdio.h>
#include <stdlib.h>
#include "Reporter.h"
#include "AudioSource.h"

class CircleSource : public AudioSource, public virtual Reporter
{
     private:

	static const int playNum = 4;//file num of the list
	static const int listPlayNum = 6;//play num of a list
	static const int oldNum = 3, newNum = 1;

	bool isOpened;
	bool musicTurn;
	bool lastFileEnd;
	bool firstCanRead;
	char fileName[255];	 
	char playList[playNum][255];
	int lastListTimes;
	int nextInList;
	int days;
	char musicName[255];
	char newDir[255],oldDir[255],musicDir[255];

	int samples;
	int bitsPerSample;
	int channel;
	double nextPlayTimeMs;
	double playTime;
 	double transBytes;
	 AudioSource *pSource;

	 int count;
	time_t nextPlayTime,curTime,beyondTime,now,initTime;

    protected:

        inline
        CircleSource ( void )                       throw ( Exception )
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
        CircleSource (  const char    * name,
                      int             sampleRate    = 44100,
                      int             bitsPerSample = 16,
                      int             channel       = 2 )
                                                        throw ( Exception )
                    : AudioSource( sampleRate, bitsPerSample, channel)
        {
            this->samples = sampleRate;
            this->bitsPerSample = bitsPerSample;
            this->channel = channel;
            init( name);
        }


      inline
        CircleSource (  const CircleSource &    ods )   throw ( Exception )
                    : AudioSource( ods )
        {
            init( ods.fileName);
        }
  	

      inline virtual
        ~CircleSource ( void )                          throw ( Exception )
        {
            strip();
        }

        inline virtual CircleSource &
        operator= (     const CircleSource &     ds )   throw ( Exception )
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

	 void prepareNextSource(void) ;

	 int  setReadRange(int startTime,int endTime=0, int elapse=0) ;

	 int genTime(char *strtime) ;
	void printTime(time_t now);
	int file_num(char * dir);
	void print_seq(int *a, int len);
	void select_n_from_old(char *oldDir, int oldFileNum, char playFileName[][255], int selectNum);
	void select_and_move_one_from_new_to_old(char *oldDir, int oldFileNum, char *newDir, int newFileNum, char *playFile);
	void select_one_from_dir(char* curDir, int dirFileNum, char* playFile);
	int make_playlist(void);



};

#endif
