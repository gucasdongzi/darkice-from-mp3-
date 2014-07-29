/*------------------------------------------------------------------------------

   Copyright (c) 2000 Tyrell Corporation.
   Copyright (c) 2006 Clyde Stubbs.

   Tyrell DarkIce

   File     : Mp3FileSource.cpp
   Version  : $Revision: 1.13 $
   Author   : $Author: darkeye $
   Location : $Source: /cvsroot/darkice/darkice/src/Mp3FileSource.cpp,v $
   
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
#include "Util.h"
#include "Exception.h"
#include "Mp3FileSource.h"
#include <errno.h>


/*
char pcmfile[][100]={"/home/languoliang/fifopcm0","/home/languoliang/fifopcm1","/home/languoliang/fifopcm2","/home/languoliang/fifopcm3","/home/languoliang/fifopcm4","/home/languoliang/fifopcm5","/home/languoliang/fifopcm6","/home/languoliang/fifopcm7","/home/languoliang/fifopcm8","/home/languoliang/fifopcm9"};
FILE* fp=fopen(pcmfile[0],"w");
unsigned int pcmindex=0;
unsigned int frameindex=0;
unsigned int frametop=(3*3600)*1000/24;//3 hour
*/
/* ===================================================  local data structures */


/* ================================================  local constants & macros */

/*------------------------------------------------------------------------------
 *  File identity
 *----------------------------------------------------------------------------*/
static const char fileid[] = "$Id$";


/*------------------------------------------------------------------------------
 *  static member
 *----------------------------------------------------------------------------*/
//static unsigned char Output[4608];
/*
 * This is a private message structure. A generic pointer to this structure
 * is passed to each of the callback functions. Put here any data you need
 * to access from within the callbacks.
 */

struct buffer 
{
	unsigned char const *start;
	unsigned long length;
	Mp3FileSource *pMp3Source;
};

/* ===============================================  local function prototypes */


/* =============================================================  module code */

/*------------------------------------------------------------------------------
 *  Tell if source id big endian
 *----------------------------------------------------------------------------*/
bool
Mp3FileSource:: isBigEndian ( void ) const                  throw ()
{
    return false;
}


/*------------------------------------------------------------------------------
 *  Initialize the object
 *----------------------------------------------------------------------------*/
void
Mp3FileSource:: init (  const char      * name )    throw ( Exception )
{
    fileName       = Util::strDup( name);
    fileDescriptor = 0;
    running        = false;
    startMSec	   = 0;
    endMSec	   = 0;
    startOffset    = 0;
    offsetLen      = 0;
    fileEnd	   = false;
    pthread_1      = 0;
    //semaphore init
    sem_init(&sem_output,0,0);
    sem_init(&sem_read,0,1);
    frameTime      = 0;
    fileTime	   = 0;	
    buffLen        = 0;
    buffhead       = 0;
    bufftail       = 0;
    outputFlag     = 0;
    outputLen      = 0;
    endFlag        = 0;
    count	   = 1;
    readOnceTime   = 0;
    initTime	   = time(NULL);
    transBytes	   = 0;
    bitRate        = 0;
    lastPage       = 0;
    curPage        = 0;
    pageSize       = 4096;
    //printf("Mp3FileSource init	endFlag = %d\n",endFlag);
}

/*------------------------------------------------------------------------------
 *  De-initialize the object
 *----------------------------------------------------------------------------*/
inline void
Mp3FileSource:: strip ( void )                      throw ( Exception )
{
    if ( isOpen() ) {
	printf("strip before close\n");
        close();
    }
    
    delete[] fileName;
	
}


/*------------------------------------------------------------------------------
 *  Open the audio source
 *----------------------------------------------------------------------------*/
bool
Mp3FileSource :: open ( void )                       throw ( Exception )
{
 
        printf("currentfile is %s \n",fileName);
	    if ( isOpen() ) 
	    {
	
        	return false;
    	}


    	if ( (fileDescriptor = ::open( fileName, O_RDONLY)) == -1 ) 
	    {
        	fileDescriptor = 0;
		    printf("fileDescriptor open error\n");
        	return false;
    	}


    	if (fstat (fileDescriptor, &st) == -1 || st.st_size == 0)
    	{
		printf("fstat error\n");
        	return false;
    	}

    	fdm = ::mmap (0, st.st_size, PROT_READ, MAP_SHARED, fileDescriptor, 0);
    	if (fdm == MAP_FAILED)
    	{
		printf("%s\n",strerror(errno));
        	return false;
    	}
	//setReadRange(0,0,0);
    	return true;
}
int 
Mp3FileSource :: getFloor(int a, int b)//get the largest interger not greater than a/b
{
        return a/b;
}
int
Mp3FileSource :: getCeil(int a, int b)//get the samllest integer not less than a/b
{
        return (a % b == 0 ? a/b : a/b+1);
}
int 
Mp3FileSource :: Mp3Locate(void)
{
	/*MpegBitRate[MepgID][LayerID][Index]*/
	unsigned int MpegBitRate[3][3][15]={ 
					      {
						{0,32,64,96,128,160,192,224,256,288,320,352,384,416,448},
						{0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384},
						{0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320}
					      },
					      {
						{0,32,48,56,64,80,96,112,128,144,160,176,192,224,256},
						{0,8, 16,24,32,40,48, 56, 64, 80, 96,112,128,144,160},
						{0,8, 16,24,32,40,48, 56, 64, 80, 96,112,128,144,160}
					      },
					      {
						{0,32,48,56,64,80,96,112,128,144,160,176,192,224,256},
						{0,8, 16,24,32,40,48, 56, 64, 80, 96,112,128,144,160},
						{0,8, 16,24,32,40,48, 56, 64, 80, 96,112,128,144,160}
					      }
					   };
	/*MpegSampleRate[MpegID][index]*/
	unsigned int MpegSampleRate[3][3]={    	
						{44100,48000,32000},
			     			{22050,24000,16000},
					       	{11025,12000, 8000}
					  };
	/*MpegSamples[MpegID][LayerID]*/
	unsigned int MpegSamples[3][3]={ 	
						{384,1152,1152},
			                        {382,1152, 576},
						{384,1152, 576}
				       };
	/*MpegLayer3SideInfo[MpegID][channelIndex]*/
       	unsigned int MpegLayer3SideInfo[3][4]={	
						{32,32,32,17},
				      		{17,17,17, 9},
				      		{17,17,17, 9}
					      };
	FILE* fp=NULL;
    	unsigned char id3v2Header[3],xingHeader[4],tocTable[100],tmp,ch;
    	unsigned int  id3v2Size,i,baseSize,samples_per_frame;
	unsigned int  MpegID = 0;//0 for MPEG1, 1 for MPEG2, 2 for MPEG2.5
	unsigned int  LayerID = 3;//0 for Layer1, 1 for Layer2, 2 for Layer3
	unsigned int  bitRateIndex,sampleRateIndex,sampleRate,channelMode,fillBit;
	unsigned int  frameNumFlag,fileSizeFlag,tocTableFlag,frameNum,tocLoc1,tocLoc2,leftTime1,leftTime2;
	//unsigned long long fileSize;
	double startPercent,endPercent,fileSize_double;
	
	fp=fopen(fileName,"r");
	fileSize_double = st.st_size;
    	/*find the location of the 1st frame*/
	id3v2Size = 0;
	fread(id3v2Header,3,1,fp);
	if (memcmp(id3v2Header,"ID3",3)==0)
	{
		fseek(fp,3,SEEK_CUR);
	/*	for(i=0;i<4;i++)
		{
			fread(&tmp,1,1,fp);
			id3v2Size <<= 7;
			id3v2Size += tmp;
		}
		fseek(fp,id3v2Size,SEEK_CUR);
		id3v2Size += 10;*/
        fread(&tmp,1,1,fp);
        id3v2Size += (tmp&0x7f)*0x200000;
        fread(&tmp,1,1,fp);
        id3v2Size += (tmp&0x7f)*0x4000;
        fread(&tmp,1,1,fp);
        id3v2Size += (tmp&0x7f)*0x80;
        fread(&tmp,1,1,fp);
        id3v2Size += tmp&0x7f;
        fseek(fp,id3v2Size,SEEK_CUR);
        id3v2Size += 10;
	}
	else 
		fseek(fp,0,SEEK_SET);
	
	/*check if it is CBR*/
       // fseek(fp,1,SEEK_CUR);//1st byte of the frameHeader

     do{
           fread(&tmp,1,1,fp);

        }while(tmp!=0xFF);//find 1st bytes of the frameHeader
	fread(&ch,1,1,fp);//2nd byte of the frameHeader
	tmp=(ch & 0x18)>>3;
	switch (tmp)
	{
		case 0 :MpegID=2;//Mpeg2.5
			break;
		case 2 :MpegID=1;//Mpeg2
			break;
		case 3 :MpegID=0;//Mpeg1
			break;
	}
	tmp=(ch & 0x06)>>1;
	switch (tmp)
	{
		case 1 :LayerID=2;//Layer3
			break;
		case 2 :LayerID=1;//Layer2
			break;
		case 3 :LayerID=0;//Layer1
			break;
	}
	fread(&tmp,1,1,fp);//3rd byte of the frameHeader
	bitRateIndex = (tmp & 0xF0)>>4;
	sampleRateIndex = (tmp & 0x0C)>>2;
	fillBit =(tmp & 0x02)>>1;
	sampleRate = MpegSampleRate[MpegID][sampleRateIndex];/*not change in VBR*/
	samples_per_frame=MpegSamples[MpegID][LayerID];/*not change in VBR*/
	frameTime = samples_per_frame*1000/sampleRate;
	printf("frameTime = %d MS\n",frameTime);
	
	fread(&tmp,1,1,fp);//4th byte of the frameHeader
	channelMode = (tmp & 0xC0)>>6;

    	fseek(fp,MpegLayer3SideInfo[MpegID][channelMode],SEEK_CUR);	
	fread(xingHeader,4,1,fp);
	if(memcmp(xingHeader,"Xing",4)==0)
	/*Now it is VBR with Xing Header*/
	{
		fseek(fp,3,SEEK_CUR);
		fread(&tmp,1,1,fp);
		frameNumFlag = (tmp & 0x01);
		fileSizeFlag = (tmp & 0x02)>>1;
		tocTableFlag = (tmp & 0x04)>>2;
		if(!frameNumFlag)
		{
			fclose(fp);
			return -1;
		}
		/*fread is LittleEndien*/
		for(i=0,frameNum=0;i<4;i++)
		{
			fread(&tmp,1,1,fp);
			frameNum = (frameNum<<8)+tmp;
		}
		if(fileSizeFlag)
			fseek(fp,4,SEEK_CUR);
		fileTime = frameTime*frameNum;
		startPercent = (double)startMSec/fileTime;
		endPercent = (double)endMSec/fileTime;
		//printf("percent=%f\n",percent);
		if(!tocTableFlag)
		{
			startOffset = (unsigned int)(startPercent*fileSize_double);
			endOffset = (unsigned int)(endPercent*fileSize_double);
		}
		else 
		{
			fread(tocTable,1,100,fp);
			tocLoc1 = (unsigned int)(startPercent*100);
			while(tocLoc1 >= 100)
				tocLoc1--;
			tocLoc2 = (unsigned int)(endPercent*100);
			while(tocLoc2 >= 100)
				tocLoc2--;
			startOffset = (unsigned int)(fileSize_double*tocTable[tocLoc1]/256);
			endOffset = (unsigned int)(fileSize_double*tocTable[tocLoc2]/256);
			leftTime1 = (startMSec-fileTime/100*tocLoc1);
			leftTime2 = (endMSec-fileTime/100*tocLoc2);
			startOffset += (unsigned int)(leftTime1*fileSize_double/fileTime);
			endOffset += (unsigned int)(leftTime2*fileSize_double/fileTime);
		}
	}

	/*Now it is CBR*/
	else
	{
		bitRate = MpegBitRate[MpegID][LayerID][bitRateIndex];
		baseSize = samples_per_frame*bitRate*1000/(sampleRate*8);
		fileTime = (unsigned int)(fileSize_double*8/bitRate);
        int offToSet = 2, curTime = 0, curPos = 0, curFrameSize = 0;
        if(startMSec >=  fileTime) {
                startOffset = st.st_size-1;
                offToSet--;
        }
        if(endMSec >= fileTime || endMSec == 0) {
                endOffset = st.st_size-1;
                offToSet--;
        }
        curPos = id3v2Size;
        fseek(fp, id3v2Size+2, SEEK_SET);
        while(offToSet) {
                fread(&tmp, 1, 1, fp);
                fillBit = (tmp & 0x02) >> 1;
                curFrameSize = baseSize + fillBit;
                if(curTime <= startMSec && curTime+frameTime > startMSec) {
                        startOffset = curPos;
                        offToSet --;
                        if(!offToSet)   
                                break;
                }
                if(curTime+frameTime >= endMSec && curTime < endMSec){
                        endOffset = curPos+curFrameSize-1;
                        offToSet --;
                        if(!offToSet)
                                break;
                }
                curTime += frameTime;
                curPos += curFrameSize;
                fseek(fp, curFrameSize, SEEK_CUR);
        }
		//startOffset = id3v2Size+baseSize*getFloor(startMSec,frameTime);//startOffset is allowed to be a little less than real value to ganrantee playtime
		//endOffset = id3v2Size+(baseSize+1)*getCeil(endMSec,frameTime);//endoffset is allowed to be a little greater than real value to ganrantee playtime
	}
	if(startOffset >= st.st_size)
		startOffset = st.st_size-1;
	if(endOffset >= st.st_size || endMSec == 0)
		endOffset = st.st_size-1;
	fclose(fp);
	return endOffset-startOffset+1;
}
int
Mp3FileSource :: setReadRange(int startTime, int endTime, int elapse)//all in ms,endTime=0 means play to the eof,return the true elapse,-1 when error
{
	startMSec = startTime;
	endMSec = endTime;
	if(startTime < 0 || endTime < 0 || elapse< 0 )
		return -1;
	if(elapse != 0)
	{
		if(endMSec == 0)
			endMSec = startMSec + elapse;
		else
			if(endMSec != startMSec + elapse)
				return -1;
	}
	offsetLen = Mp3Locate();
    if(endMSec > fileTime||endMSec == 0)
               endMSec = fileTime;
	printf("startMSec=%d  endMSec=%d\n",startMSec,endMSec);
	printf("fSize=%lu startOffset=%d  endOffset=%d  offlen=%d\n",st.st_size,startOffset,endOffset,offsetLen);
	if(offsetLen == -1)
		return -1;
    	//create thread
  	int ret1;
  	ret1 = pthread_create(&pthread_1,NULL,(void *(*)(void *))pthread_decode,this);
  	if (ret1 != 0)
  	{
  		printf("create decode thread failure\n");
  	}
        //readOnceTime = 4096*frameTime/(1152*2*2);
 	return endMSec-startMSec;	
}


/*------------------------------------------------------------------------------
 *  Check whether read() would return anything
 *----------------------------------------------------------------------------*/
bool
Mp3FileSource :: canRead ( unsigned int    sec,
                          unsigned int    usec )    throw ( Exception )
{

	
	if ( !isOpen() ) 
	{
        	return false;
    	}
	if( fileEnd )
	{
		return false;
	}
        //printf("canRead\n");
	/*
        playTime = transBytes/(1152*2*2)*frameTime/1000;
        now = time(NULL);
        if (playTime+initTime-now > 3)
                usleep(40*1000);
        else
                usleep(10*1000);
	*/
    	return true;
}

/*------------------------------------------------------------------------------
 *  Read from the audio source
 *----------------------------------------------------------------------------*/
unsigned int
Mp3FileSource :: read (    void          * buf,
                          unsigned int    len )     throw ( Exception )
{
	ssize_t          ret;
    	unsigned char    * ptr;
    	if ( !isOpen() ) 
	{
        	return 0;
    	}
    	ret = 0;
	unsigned int i, j, end, outputCurrent;
	ptr = (unsigned char*)buf;
        if(endFlag == 1)
        {

		unsigned int length;
		length = buffLen > len ? len : buffLen;
		
		
		for(end = 0;end < length;end++)
                {
                        *(ptr++) = ubuff[buffhead];
                        buffhead = (buffhead + 1) % sizeof(ubuff);
                        ret++;
                	
		}
		
                buffLen = (bufftail - buffhead + sizeof(ubuff)) % sizeof(ubuff);
		transBytes += ret;
                return ret;

        }       
	while(1)
	{
			
		OutputPtr = Output;
		ptr = (unsigned char *)buf;
		
		sem_wait(&sem_read);

		buffLen = (bufftail - buffhead + sizeof(ubuff)) % sizeof(ubuff);
		if(buffLen >= len)
		{
			for(i=0;i<len;i++)
			{
				*(ptr++) = ubuff[buffhead];
				buffhead = (buffhead + 1) % sizeof(ubuff);
				ret++;
			}
			buffLen = (bufftail - buffhead + sizeof(ubuff)) % sizeof(ubuff);
			ptr = (unsigned char *)buf;
			sem_post(&sem_read);
			break;
		}
		else
		{
			if(outputFlag == 0)
			{
				sem_post(&sem_output);
				outputFlag = 1;
			}
			else
			{
				for(i=0;i<buffLen;i++)
				{
					*(ptr++) = ubuff[buffhead];
					buffhead = (buffhead + 1) % sizeof(ubuff);
					ret++;
				}

				j = i;
				for(;j < len;j++)
				{
					*(ptr++) = *(OutputPtr++);
					ret++;
				}
				outputCurrent = OutputPtr - Output; 
				for(;outputCurrent<outputLen;outputCurrent++)
				{
					ubuff[bufftail] = *(OutputPtr++);
					bufftail = (bufftail + 1) % sizeof(ubuff);
				}
				buffLen = (bufftail - buffhead + sizeof(ubuff)) % sizeof(ubuff);
				outputFlag = 0;
				ptr = (unsigned char *)buf;

				sem_post(&sem_read);
				break;
			}
		}
		OutputPtr = Output;
		ptr = (unsigned char *)buf;
	}

    	running = true;
	transBytes += ret;
    curPage = (int)((transBytes*bitRate*1000/pcmBitRate+startOffset)/pageSize-2);
    // printf("%d  %d  %f  %d  %d  %d  %d\n", lastPage, curPage, transBytes, bitRate, pcmBitRate, startOffset, pageSize);
    if(curPage > lastPage) {
	    char *fdmc = (char *)fdm;
            ::munmap(fdmc+lastPage*pageSize, (curPage-lastPage)*pageSize);
    //      printf("%d  %d\n", lastPage, curPage);
            lastPage = curPage;
    }
   
    return ret;
}


/*------------------------------------------------------------------------------
 *  Close the audio source
 *----------------------------------------------------------------------------*/
void
Mp3FileSource :: close ( void )                  throw ( Exception )
{
    if ( !isOpen() ) {
	printf("close when not open\n");
        return;
    }
    pthread_join(pthread_1,NULL);
    ::munmap(fdm,st.st_size);
    ::close(fileDescriptor);
    fileDescriptor = 0;
    running        = false;
}

/*------------------------------------------------------------------------------
 *  decode mp3 source
 *----------------------------------------------------------------------------*/
void *Mp3FileSource::pthread_decode(void *param)
{
	Mp3FileSource *pThis = (Mp3FileSource*)param;
//   printf("now pthread started,endFlag = %d\n",pThis->endFlag);
	pThis->decode((unsigned char*)(pThis->fdm) + pThis->startOffset, pThis->offsetLen, param);
	pthread_detach(pthread_self());
	return 0;
}

/*
 * This is the input callback. The purpose of this callback is to (re)fill
 * the stream buffer which is to be decoded. In this example, an entire file
 * has been mapped into memory, so we just call mad_stream_buffer() with the
 * address and length of the mapping. When this callback is called a second
 * time, we are finished decoding.
 */

enum mad_flow Mp3FileSource::input(void *data,
		    struct mad_stream *stream)
{
  struct buffer *buffer = (struct buffer*)data;
  if (!buffer->length)
    return MAD_FLOW_STOP;
  mad_stream_buffer(stream, buffer->start, buffer->length);
  buffer->length = 0;

  return MAD_FLOW_CONTINUE;
}

/*
 * The following utility routine performs simple rounding, clipping, and
 * scaling of MAD's high-resolution samples down to 16 bits. It does not
 * perform any dithering or noise shaping, which would be recommended to
 * obtain any exceptional audio quality. It is therefore not recommended to
 * use this routine if high-quality output is desired.
 */

inline
signed int Mp3FileSource::scale(mad_fixed_t sample)
{
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);

}

/*
 * This is the output callback function. It is called after each frame of
 * MPEG audio data has been completely decoded. The purpose of this callback
 * is to output (or play) the decoded PCM audio.
 */

enum mad_flow Mp3FileSource::output(void *data,
		     struct mad_header const *header,
		     struct mad_pcm *pcm)
{
    struct buffer *buffer = (struct buffer*)data;

    sem_wait(&buffer->pMp3Source->sem_output);

    unsigned int nchannels, nsamples;
    int slen=0;
    mad_fixed_t const *left_ch, *right_ch;
    /*
if (frameindex == frametop){
	pcmindex++;
	fclose(fp);
	if((fp=fopen(pcmfile[pcmindex],"w"))==NULL)
		printf("%s open error\n",pcmfile[pcmindex]);
	frameindex=0;
    }
   frameindex++;
   */
    nchannels    = pcm->channels;
    nsamples = pcm->length;
    /*
    if(nsamples != 1152)
   	 printf("nasamples error %d\n",nsamples);
    if(nchannels != 2)
   	 printf("nchannels error %d\n",nchannels);
    */
    left_ch      = pcm->samples[0];
    right_ch     = pcm->samples[1];
    buffer->pMp3Source->OutputPtr = buffer->pMp3Source->Output;

    while (nsamples--)
    {
			signed int sample;
			sample = scale(*left_ch++);
			*(buffer->pMp3Source->OutputPtr++) = (sample >> 0)&0xff;
			*(buffer->pMp3Source->OutputPtr++) = (sample >> 8)&0xff;
			slen=slen+2;
			if (nchannels == 2) 
			{
				sample = scale(*right_ch++);
				*(buffer->pMp3Source->OutputPtr++) = (sample >> 0)&0xff;
				*(buffer->pMp3Source->OutputPtr++) = (sample >> 8)&0xff;
				slen=slen+2;
			}
    }
    //fwrite(buffer->pMp3Source->Output,1,slen,fp);
    //buffer->pMp3Source->outputLen = buffer->pMp3Source->OutputPtr - buffer->pMp3Source->Output;
    buffer->pMp3Source->outputLen = slen;
    //printf("output length = %d\n",buffer->pMp3Source->outputLen);
    buffer->pMp3Source->OutputPtr = buffer->pMp3Source->Output;

    sem_post(&buffer->pMp3Source->sem_read);

    return MAD_FLOW_CONTINUE;
}

/*
 * This is the error callback function. It is called whenever a decoding
 * error occurs. The error is indicated by stream->error; the list of
 * possible MAD_ERROR_* errors can be found in the mad.h (or stream.h)
 * header file.
 */

enum mad_flow Mp3FileSource::error(void *data,
		    struct mad_stream *stream,
		    struct mad_frame *frame)
{
/*
  struct buffer *buffer = data;
  fprintf(stderr, "decoding error 0x%04x (%s) at byte offset %u\n",
	  stream->error, mad_stream_errorstr(stream),
	  stream->this_frame - buffer->start);
*/
  /* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */
  return MAD_FLOW_CONTINUE;
}

/*
 * This is the function called by main() above to perform all the decoding.
 * It instantiates a decoder object and configures it with the input,
 * output, and error callback functions above. A single call to
 * mad_decoder_run() continues until a callback function returns
 * MAD_FLOW_STOP (to stop decoding) or MAD_FLOW_BREAK (to stop decoding and
 * signal an error).
 */

int Mp3FileSource::decode(unsigned char const *start, unsigned long length, void *param)
{
  struct buffer buffer;
  struct mad_decoder decoder;
  int result;
  /* initialize our private message structure */
  buffer.start  = start;
  buffer.length = length;
  buffer.pMp3Source = (Mp3FileSource*)param;
  /* configure input, output, and error functions */
  mad_decoder_init(&decoder, &buffer,
		   input, 0 /* header */, 0 /* filter */, output,
		   error, 0 /* message */);
 // mad_decoder_options(&decoder, 0);   //add
  /* start decoding, if success result = 0 */
  result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
  /* release the decoder */
  mad_decoder_finish(&decoder);

  buffer.pMp3Source->endFlag = 1;
  struct tm *tblock;
  time_t now = time(NULL);
  tblock = localtime(&now);
  //printf("the time to change endFlag to 1 is %s",asctime(tblock));
  //printf("pthread ID = %lu\n",pthread_self());
  return result;
}
