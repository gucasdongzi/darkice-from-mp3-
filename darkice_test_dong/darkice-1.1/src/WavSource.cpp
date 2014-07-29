#include "WavSource.h"


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

#define htoll(b)  \
          (((((b)      ) & 0xFF) << 24) | \
	       ((((b) >>  8) & 0xFF) << 16) | \
		   ((((b) >> 16) & 0xFF) <<  8) | \
		   ((((b) >> 24) & 0xFF)      ))
#define htols(b) \
          (((((b)      ) & 0xFF) << 8) | \
		   ((((b) >> 8) & 0xFF)      ))
#define ltohl(b) htoll(b)
#define ltohs(b) htols(b)

#include "Util.h"
#include "Exception.h"


bool
WavSource :: isBigEndian ( void ) const                  throw ()
{
      return false;
}


void
WavSource :: init (  const char      * name )    throw ( Exception )
{
    fileName       = Util::strDup( name);
    fileDescriptor = 0;
    running        = false;
}


void
WavSource :: strip ( void )                      throw ( Exception )
{
    if ( isOpen() ) {
        close();
    }
    
    delete[] fileName;
}


int
WavSource::check_header(FILE *f)
{
	int type, size, formtype;
	int fmt, hsize;
	short format, chans, bysam, bisam;
	int bysec;
	int freq;
	int data;

	
	if (fread(&type, 1, 4, f) != 4) {
		reportEvent(3, "Read failed (type)\n");
		return -1;
	}
	
	if (fread(&size, 1, 4, f) != 4) {
		reportEvent(3, "Read failed (size)\n");
		return -1;
	}
	size = ltohl(size);
	
	if (fread(&formtype, 1, 4, f) != 4) {
		reportEvent(3, "Read failed (formtype)\n");
		return -1;
	}
	
	if (memcmp(&type, "RIFF", 4)) {
		reportEvent(3, "Does not begin with RIFF\n");
		return -1;
	}
	if (memcmp(&formtype, "WAVE", 4)) {
		reportEvent(3, "Does not contain WAVE\n");
		return -1;
	}
	if (fread(&fmt, 1, 4, f) != 4) {
		reportEvent(3, "Read failed (fmt)\n");
		return -1;
	}
	if (memcmp(&fmt, "fmt ", 4)) {
		reportEvent(3, "Does not say fmt\n");
		return -1;
	}
	if (fread(&hsize, 1, 4, f) != 4) {
		reportEvent(3, "Read failed (formtype)\n");
		return -1;
	}
	if (hsize < 16) {
		reportEvent(3, "Unexpected header size %d\n", ltohl(hsize));
		return -1;
	}
	if (fread(&format, 1, 2, f) != 2) {
		reportEvent(3, "Read failed (format)\n");
		return -1;
	}
	if (format != 1) {
		reportEvent(3, "Not a wav file", format);
		return -1;
	}
	if (fread(&chans, 1, 2, f) != 2) {
		reportEvent(3, "Read failed (format)\n");
		return -1;
	}
	if (chans != 2) {
		reportEvent(3, "Not in mono %d\n", ltohs(chans));
		return -1;
	}

	if (fread(&freq, 1, 4, f) != 4) {
		reportEvent(3, "Read failed (freq)\n");
		return -1;
	}
	
	/* Ignore the byte frequency */
	if (fread(&bysec, 1, 4, f) != 4) {
		reportEvent(3, "Read failed (BYTES_PER_SECOND)\n");
		return -1;
	}
	/* Check bytes per sample */
	if (fread(&bysam, 1, 2, f) != 2) {
		reportEvent(3, "Read failed (BYTES_PER_SAMPLE)\n");
		return -1;
	}
	//if (ltohs(bysam) != 2) {
	//	reportEvent(3, "Can only handle 16bits per sample: %d\n", ltohs(bysam));
	//	return -1;
	//}

       
	
	if (fread(&bisam, 1, 2, f) != 2) {
		reportEvent(3, "Read failed (Bits Per Sample): %d\n", ltohs(bisam));
		return -1;
	}
       //bitsPerSample = ltohs(bisam);
	
	/* Skip any additional header */
	if (fseek(f,hsize-16,SEEK_CUR) == -1 ) {
		reportEvent(3, "Failed to skip remaining header bytes: %d\n", hsize-16 );
		return -1;
	}
	/* Skip any facts and get the first data block */
	for(;;)
	{ 
		char buf[4];
	    
	    /* Begin data chunk */
	    if (fread(buf, 1, 4, f) != 4) {
			reportEvent(3, "Read failed (data)\n");
			return -1;
	    }
	    /* Data has the actual length of data in it */
	    if (fread(&data, 1, 4, f) != 4) {
			reportEvent(3, "Read failed (data)\n");
			return -1;
	    }
	    data = data;
	    if(memcmp(buf, "data", 4) == 0 ) 
			break;
	    if(memcmp(buf, "fact", 4) != 0 ) {
			reportEvent(3, "Unknown block - not fact or data\n");
			return -1;
	    }
	    if (fseek(f,data,SEEK_CUR) == -1 ) {
			reportEvent(3, "Failed to skip fact block: %d\n", data );
			return -1;
	    }
	}

	return data;
}



bool
WavSource :: open ( void )                       throw ( Exception ){

    if ( isOpen() ) {
        return false;
    }

   fileDescriptor = fopen(fileName,"rb");

   if(fileDescriptor == NULL)
   	  throw Exception( __FILE__, __LINE__, "can't open wav file");

   if(check_header(fileDescriptor) < 0)
   	   return false;
   return true;	
}

bool
WavSource:: canRead ( unsigned int    sec,
                          unsigned int    usec )    throw ( Exception )
{

   if ( !isOpen() ) {
        return false;
   }	
   usleep(16000);
   
   return true; 	
	
}


unsigned int
WavSource :: read (    void          * buf,
                          unsigned int    len )     throw ( Exception )
{

    ssize_t         ret;

    if ( !isOpen() ) {
        return 0;
    }	
   
   if(::feof(fileDescriptor)){
            close();
            return 0;
   }
        
    ret = fread(buf,1,len,fileDescriptor);
    running = true;
	
    return ret;    

}


void
WavSource :: close ( void )                  throw ( Exception )
{
    if ( !isOpen() ) {
        return;
    }

    ::fclose( fileDescriptor);
    fileDescriptor = 0;
    running        = false;
}



