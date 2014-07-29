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

#include <iostream>
#include <fstream>
#include "EpgSource.h"
#include "WavSource.h"
#include "OssDspSource.h"
#include "Mp3FileSource.h"
#include "DarkIceConfig.h"

bool
EpgSource :: isBigEndian ( void ) const                  throw ()
{
      return false;
}


void
EpgSource :: init (  const char      * name )    throw ( Exception )
{    

       std::ifstream       configFile("/etc/darkice.cfg.2");
	Config              config( configFile);
	const ConfigSection    * cs;
       const char             * str;


	isOpened=false;	   
       pSource =NULL;
	 count = 0;

	 if ( !(cs = config.get( "general")) ) {
                     throw Exception( __FILE__, __LINE__, "no section [general] in mysql config");
        }

        str = cs->getForSure( "dbhost", " missing in section [general]");

	 sprintf(dbhost,"%s",str);

	  str = cs->getForSure("dbname","missing in section [general]");

	  sprintf(dbname,"%s",str);

	  str = cs->getForSure("dbtable","missing in section [general]");

	  sprintf(dbtable,"%s",str);

	  str = cs->getForSure("dbuser","missing in section [general]");

	  sprintf(dbuser,"%s",str);


         str = cs->getForSure("dbpasswd","missing in section [general]");

	  sprintf(dbpasswd,"%s",str);
	  
         mysql_init(&mysql);


	  if (!mysql_real_connect(&mysql, dbhost, dbuser, dbpasswd,dbname, 0, 0, 0)) {
		reportEvent(3, "Fail  to connect to Mysql Database",mysql_error(&mysql));
		connected = false;
		return;
	  } else {
		reportEvent(3, "Mysql Database is connected.\n");
		connected = true;		
	  }

	 prepareNextSource();
	
}
int EpgSource::genTime(char *strtime){

      struct tm *ptr; 
      time_t          now;
      int hour,minute,seconds;

      if(strtime ==NULL)
	  	return 0;

      now=time(NULL);	
      sscanf(strtime,"%2d:%2d:%2d",&hour,&minute,&seconds);
      ptr = localtime(&now);
      
      ptr->tm_hour = hour;
      ptr->tm_min = minute;
      ptr->tm_sec = seconds;
     
      return mktime(ptr);

}
void
EpgSource::prepareNextSource(void){


         char          strTime[8];
	 char	       fileStrTime[]="00:00:00";
	  struct tm *ptr; 
         time_t          now;
	  char query[256];
	  MYSQL_RES *result;
	  MYSQL_ROW row;   

         now = time(NULL);
	  ptr = localtime(&now);
	  strftime(strTime,sizeof(strTime),"%R",ptr);
		 
         sprintf(query,"select StartTime,EndTime,Elapse,SourceType,FileName,Samples,BitsPerSample,Channel,FileStartTime,FileEndTime from %s where StartTime <= '%s' and EndTime >= '%s'",dbtable,strTime,strTime);

	  mysql_query(&mysql,query);// need consider whether the connection to mysql  is timeout, we should do some test to make sure the connection is ok.

	  result =mysql_store_result(&mysql);

	   if(result){
                row = mysql_fetch_row(result);
	        if(row){

                         if(row[0]!=NULL)
				startTime = genTime(row[0]);

			    if(row[1]!=NULL)
				 endTime = genTime(row[1]);

			    if((endTime > 0) && (startTime > endTime))
					endTime += 86400;

			    reportEvent(3, "StartTime:",startTime," Endtime: ",endTime);

			    if(row[2]!=NULL)
				     elapse = atoi(row[2]);

			    if(row[3]!=NULL)
				 sprintf(sourceType,"%s",row[3]);

			    if(row[4]!=NULL)
				 sprintf(fileName,"%s",row[4]);
                         
			    if(row[5]!=NULL)
				     sampleRate = samples = atoi(row[5]);

			    if(row[6]!=NULL)
				     bitsPerSample= atoi(row[6]);

			    if(row[7]!=NULL)
				     channel = atoi(row[7]);

			    fileStartTime = genTime(row[8]);

			    fileEndTime = genTime(row[9]);

			    fileHeadTime = genTime(fileStrTime);
			   	
		}

		mysql_free_result(result);
			
	   }


        if(!strcasecmp(sourceType,"oss")){
			
               Reporter::reportEvent( 1, "Using OSS DSP input device:", fileName);
               pSource = new OssDspSource(fileName,samples,bitsPerSample,channel);
               //pSource->open();
		//pSource->setReadRange(0,0,0);

	}else if(!strcasecmp(sourceType,"mp3")){
	
               Reporter::reportEvent( 1, "Using Mp3 File:", fileName);
               pSource =  new Mp3FileSource(fileName,samples,bitsPerSample,channel); 
	       //printf("FileStartTime=%ld,  FileEndTime=%ld,  FileHead=%ld\n",fileStartTime,fileEndTime,fileHead);
               //pSource->open();
	       //setReadRange(fileStartTime-fileHead,fileEndTime-fileHead,fileEndTime-fileStartTime);
	       
              
	}else if(!strcasecmp(sourceType,"s48")){

               Reporter::reportEvent( 1, "Using S48 File:", fileName);
               pSource = new Mp3FileSource(fileName,samples,bitsPerSample,channel); 
               //pSource->open();

	}else if(!strcasecmp(sourceType,"wav")){

               Reporter::reportEvent( 1, "Using WAV File:", fileName);
               pSource = new WavSource(fileName,samples,bitsPerSample,channel); 
               //pSource->open();
			   
	}

}

bool 
EpgSource::setReadRange(int startTime,int endTime, int elapse){

    //printf("FileStartTime=%d,  FileEndTime=%d,  elapse=%d\n",startTime,endTime,elapse);
    return pSource->setReadRange(startTime,endTime,elapse);
}

void
EpgSource :: strip ( void )                      throw ( Exception )
{
    if ( isOpen() ) {
        close();
    }
    
}

bool
EpgSource :: open ( void )                       throw ( Exception ){

    if ( isOpen() ) {
        return false;
    }
  
    //prepareNextSource();
   pSource->open();
   if(strcasecmp(sourceType,"mp3"))
	setReadRange(fileStartTime-fileHeadTime,fileEndTime-fileHeadTime,fileEndTime-fileStartTime);
   isOpened=true; 
   return true;	

}

bool
EpgSource:: canRead ( unsigned int    sec,
                          unsigned int    usec )    throw ( Exception )
{

    if ( !isOpen() ) {
        return false;
    }	

    return pSource->canRead(sec,usec);
	

}

unsigned int
EpgSource :: read (    void          * buf,
                          unsigned int    len )     throw ( Exception )
{

    ssize_t         ret;
    time_t          now;

    if ( !isOpen() ) {
        return 0;
    }	
    if(count == 30){
        now = time(NULL);
        if(now >= endTime){
             close();
	      open();
	}
	count = 0;
    }
    
    ret =  pSource->read(buf,len);
    count ++;

   return ret;

}


void
EpgSource :: close ( void )                  throw ( Exception )
{
    if ( !isOpen() ) {
        return;
    }
    pSource->close();
    delete pSource;
    pSource = NULL;

}


