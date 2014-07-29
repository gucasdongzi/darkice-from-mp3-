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
#include <dirent.h>
#include "EpgSource.h"
#include "WavSource.h"
#include "OssDspSource.h"
#include "Mp3FileSource.h"
#include "DarkIceConfig.h"
#include "JackDspSource.h"

char filter_sourceType[][5]={"s48", "mp3"};
int  filter_sourceType_len = 2;

#define fgets2(buf, n, fp); fgets((buf), (n), (fp));\
                            while(buf[strlen(buf)-1] == '\n')\
                                buf[strlen(buf)-1] = '\0';
bool
EpgSource :: isBigEndian ( void ) const                  throw ()
{
    return false;
}


void
EpgSource :: init (  const char      * name )    throw ( Exception )
{
    std::ifstream       configFile(cfgfile);
    Config              config( configFile);
    const ConfigSection    * cs;
    const char             * str;

    isOpened = false;
    pSource = NULL;
    count = 0;
    elapseLeft = 0;
    started =false;
    transBytesTotal = 0;
    transBytesCur = 0;
    lastFileEnd = true; 
    playTime = 0;
    mysqlconnected = false;
    setvbuf(stdout, NULL, _IONBF, 0);
    mp3_samples=0;
    mp3_bitsPerSample=0;
    mp3_channel=0; 
    lastFileEnd=true;
    if ( !(cs = config.get( "general")) ) {
	throw Exception( __FILE__, __LINE__, "no section [general] in mysql config");
    }

    str = cs->getForSure( "dbhost", " missing in section [general]");
    sprintf(dbhost,"%s",str);

    str = cs->getForSure("dbname","missing in section [general]");
    sprintf(dbname,"%s",str);

	//str = cs->getForSure("dbtable","missing in section [general]");
	//sprintf(dbtable,"%s",str);

    //sprintf(dbtable, "%s", epgTable);

    str = cs->getForSure("dbuser","missing in section [general]");
    sprintf(dbuser,"%s",str);

    str = cs->getForSure("dbpasswd","missing in section [general]");
    sprintf(dbpasswd,"%s",str);

    str = cs->getForSure("pathcfg","missing in section [general]");
    sprintf(pathcfg,"%s",str);

    str = cs->getForSure("blankPath","missing in section [general]");
    sprintf(blankPath,"%s",str);

    str = cs->getForSure("minTime","missing in section [general]");
    minTime = atoi(str);

    //sprintf(blankFileName, "%s/%d.%d.%d.%d.mp3", blankPath, samples, bitsPerSample, channel, minTime);
    sprintf(blankFileName, "%s/blank.mp3", blankPath);

    if(!check_mysql_connect(&mysql)) {
	reportEvent(3, "Mysql Database is connected.\n");
   
    }
}
time_t
EpgSource::genTime(char *strtime, time_t now){

    struct tm *ptr;
    int hour,minute,seconds;

    if(!strtime)
	    return 0;
    sscanf(strtime,"%2d:%2d:%2d",&hour,&minute,&seconds);
    ptr = localtime(&now);
    ptr->tm_hour = hour;
    ptr->tm_min = minute;
    ptr->tm_sec = seconds;
    return mktime(ptr);
}

int
filter(const struct dirent* file)
{
    struct stat statbuf;
    char *suffix;
    char file_d_name[256];

    lstat(file->d_name, &statbuf);
    if(!S_ISREG(statbuf.st_mode))
	    return 0;

    snprintf(file_d_name, 256, "%s", file->d_name);
    suffix = strrchr(file_d_name, '.');
    if(suffix == NULL)
	    return 0;
    suffix++;
    for(int i = 0; i < filter_sourceType_len; i++)
	    if(!strcmp(suffix, filter_sourceType[i]))
	        return 1;

    return 0;
}
int
EpgSource::newPathcfg(FILE *fp)
{
     char *p,fileNames[256];
     int i=0,j=0,k=0;
     while(fileName[i]!='\0')
     {fileNames[j++]=fileName[k++];
     i++;
     }
     fileNames[i]='\0';
     i=0;
     printf("filenames :%s filename:%s\n",fileNames,fileName);
     p=strtok(fileNames,",");
     if(p){
     sprintf(nextFileName,"%s/%s@%d",filePath,p,i);
     fprintf(fp,"CurrentFile %s@%d\n",p,i);
     fprintf(fp, "FileList:\n");
     fprintf(fp,"%s@%d\n",p,i);
           }
           else{

           return -1;
           }
    do{
        i++;

       p=strtok(NULL,",");
       fprintf(fp,"%s@%d\n",p,i);
       //fprintf(fp,"%s/n","dadwed");
        printf("filelist is:%s@%d\n",p,i);
       }while(p);
       printf("newPathcfg is ok");
       return 0;
}
int EpgSource::gets_b_c(char *name)
{

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


	FILE* fp=NULL;
    unsigned char id3v2Header[3],tmp,ch;
        unsigned int  id3v2Size,i,samples_per_frame;
	unsigned int  MpegID = 0;//0 for MPEG1, 1 for MPEG2, 2 for MPEG2.5
	unsigned int  LayerID = 3;//0 for Layer1, 1 for Layer2, 2 for Layer3
	unsigned int  bitRateIndex,sampleRateIndex,sampleRate,channelMode,fillBit;

	//unsigned long long fileSize;
	//double startPercent,endPercent,fileSize_double;

	fp=fopen(name,"r");
	if (fp==NULL){
	return 0;
	}
	//fileSize_double = st.st_size;
    	/*find the location of the 1st frame*/
	id3v2Size = 0;
	fread(id3v2Header,3,1,fp);
	if (memcmp(id3v2Header,"ID3",3)==0)
	{
		 fseek(fp,3,SEEK_CUR);
		//for(i=0;i<4;i++)
		//{
			fread(&tmp,1,1,fp);
            printf("tmp ---%d\n",tmp);
			id3v2Size += (tmp&0x7f)*0x200000;
            fread(&tmp,1,1,fp);
            printf("tmp -----%d\n",tmp);
            id3v2Size += (tmp&0x7f)*0x4000;
            fread(&tmp,1,1,fp);
            printf("tmp----%d\n",tmp);
            id3v2Size += (tmp&0x7f)*0x80;
            fread(&tmp,1,1,fp);
            printf("tmp----%d\n",tmp);
            id3v2Size += tmp&0x7f;

		//}
		fseek(fp,id3v2Size,SEEK_CUR);
        
        
        printf("id3v2size %d\n",id3v2Size);
		id3v2Size += 10;
	}
	else
		fseek(fp,0,SEEK_SET);
    
   do{
       fread(&tmp,1,1,fp);
     
     }while(tmp!=0xFF);


	   /*check if it is CBR*/
       // fseek(fp,1,SEEK_CUR);//1st byte of the frameHeader

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
    bitsPerSample=16;

	fread(&tmp,1,1,fp);//4th byte of the frameHeader
	channelMode = (tmp & 0xC0)>>6;
    fclose(fp);
    printf("sampleRate:%d,chanelmode:%d,bitsPerSample:%d \n",sampleRate,channelMode,bitsPerSample);
    if(sampleRate!=0&&bitsPerSample!=0)
    {
        mp3_bitsPerSample=bitsPerSample;

        if(channelMode==2)
            {mp3_channel=1;}
        else
        {
            mp3_channel=2;
        }
        mp3_samples=sampleRate;
        printf("mp3_sample is ok\n");
        return 1;


    }
    else
    {

        return 0;
    }


}
int
EpgSource::getFileFromPath(void){

    FILE *fp, *ftmp;
    int rc;
    char *index;
    char buf[1024];
    char buf1[1024];
    char lastFile[256];
    bool listEnd;

    if(chdir(filePath)){
	printf("chdir %s error\n", filePath);
        return -1;
    }
    if((fp = fopen(pathcfg,"r")) == NULL){
	fp = fopen(pathcfg,"w");
        if(fp){
	    printf("create new pathcfg in %s success\n", filePath);
	    rc = newPathcfg(fp);
	    fclose(fp);
	    return rc;
	}else{
	    printf("create new pathcfg in %s failed\n", filePath);
            return -1;
        }
    }else{
	printf("filepath exist, now get file\n");
        fgets2(buf, 1024, fp);
        if(strncmp(buf, "CurrentFile ", strlen("CurrentFile "))){
	    printf("pathcfg error in %s\n", filePath);
            fclose(fp);
            remove(pathcfg);
            return -1;
        }
        sprintf(lastFile, "%s", buf + strlen("CurrentFile "));
       // printf("lastFile=%s\n",lastFile);
        while(1){
	    fgets2(buf, 1024, fp);
            if(!strcmp(buf, lastFile))
		break;
            if(feof(fp)){
		fclose(fp);
                fp = fopen(pathcfg, "w");
                rc = newPathcfg(fp);
                fclose(fp);
                return rc;
            }
        }
        listEnd = true;
        while(1){
	    fgets2(buf, 1024, fp);
            if(feof(fp))
		break;
           // printf("buf = %s\n",buf);
            sprintf(buf1,"%s",buf);
            index=strstr(buf1,"@");
            if(index!=NULL) *index='\0';
            ftmp = fopen(buf1, "r");
            if(ftmp){
		fclose(ftmp);
                listEnd = false;
                sprintf(nextFileName, "%s/%s", filePath, buf);
                break;
            }
        }
        ftmp = fopen("cfgtmp", "w");
	if(listEnd){
	    printf("another list circle\n");
            newPathcfg(ftmp);
        }else{
            fprintf(ftmp, "CurrentFile %s\n", buf);
            rewind(fp);
            fgets2(buf, 1024, fp);
            while(1){
		fgets2(buf, 1024, fp);
                if(feof(fp))
		    break;
                fprintf(ftmp, "%s\n", buf);
            }
        }
        fclose(ftmp);
        fclose(fp);
        if(rename("cfgtmp", pathcfg) == -1){
	    printf("rename failed\n");
            return -1;
        }
    }
    return 0;

}

int
EpgSource::prepareNextSource(void)
{
    int rc;
    char *needle;

    memset(nextFileName, 0, sizeof(nextFileName));
    printTime(time(NULL));
    Reporter::reportEvent(1,"Now Play A New Source");

    if(elapseLeft <= 0){
	    
	Reporter::reportEvent(1,"Now Play A New Program");
        rc = prepareNextProgram();
        char cfg[256];
        sprintf(cfg,"%s/%s",filePath,pathcfg);
        if(remove(cfg)==0)
			{
			 printf("remove pathcfg \n");
			}
        //if(rc){
	        //printf("prepareNextProgram failed\n");
	      //  return -1;
            //}
        if(!started){
           printf("the value of started is false \n");
            gettimeofday(&initTime, NULL);
            startTime = initTime.tv_sec;
           if(rc==0){// preparen next program success
                Reporter::reportEvent(1,"PrepareNextProgram Success");
		 elapseLeft += (endTime - startTime)*1000 - initTime.tv_usec/1000;
                }else{// preparen next program filad
		 prepareBlankProgram();//prepare blank program
                 elapseLeft += elapse*1000;
		}
	    started = true;
        }else{  printf("program has started........\n");
	        elapseLeft += elapse*1000;
        }
    }
    printf("elapseLeft=%f\n", elapseLeft);
    /*if(fileName[0]){
	    sprintf(nextFileName, "%s", fileName);
    }else if(filePath[0]){
        getFileFromPath();
    }*/
   if(isDevice==0){//if not Device get file from Path
    if(filePath[0])
    {
       if( getFileFromPath()==-1)
       {
           prepareBlankProgram();
           //don't get file from Path play blankfile

        // printf("%d getfilefrom\n",getFileFromPath());
    
    //filePath is not null ,which shows that the sourceType is mp3 or s48
   	}else{ char *a;
   		 a=strstr(nextFileName,"@");
    		if(a!=NULL) *a='\0';
    		//printf("nextfilename =%s\n",nextFileName);

        	needle = strrchr(nextFileName, '.');
       		 needle++;
   		}	 
	} else{
 	 //prepareBlankProgram();//front has preparing.....
	 needle = sourceType;
	}
    }else{
        needle = sourceType;
        sprintf(nextFileName, "%s","jack");
    }
    printf("new NextSource needle is :%s\n",needle);

    newNextSource(needle, nextFileName);
    printf("new source open");
    if(!pSource->open()){
        Reporter::reportEvent(1,"Warning.....","Source can't openplay blank file");
        pSource->strip();
        sprintf(nextFileName, "%s", blankFileName);
        newNextSource("mp3", nextFileName);
        if(!pSource->open()){
            pSource->strip();
            close();
        }
    }
  printf("isdev");
    if(!isdev)
	    setReadRange(0, 0, (int)(elapseLeft+1));
    return 0;
}
void
EpgSource::newNextSource(char *sourceType, char *nextFileName)
{
    if(!strcasecmp(sourceType,"mp3")){
	    isdev = false;
	    //printf("get %d",gets_b_c(nextFileName));
	    if(gets_b_c(nextFileName)){
    	Reporter::reportEvent( 1, "Using MP3 File:", nextFileName);
        //pSource = new Mp3FileSource(nextFileName,samples,bitsPerSample,channel);
        pSource = new Mp3FileSource(nextFileName,mp3_samples,mp3_bitsPerSample,mp3_channel);
	    }
	    else
	    pSource = new Mp3FileSource(nextFileName,samples,bitsPerSample,channel);
    }else if(!strcasecmp(sourceType,"s48")){
        isdev = false;
         if(gets_b_c(nextFileName)){
        Reporter::reportEvent( 1, "Using S48 File:", nextFileName);
        printf("Using s48 file %s\n",nextFileName);
        pSource = new Mp3FileSource(nextFileName,mp3_samples,mp3_bitsPerSample,mp3_channel);
         }
         else
         {
             Reporter::reportEvent( 1, "Using S48 File:", nextFileName);

        pSource = new Mp3FileSource(nextFileName,samples,bitsPerSample,channel);
         }
    }else if(!strcasecmp(sourceType,"wav")){
        isdev = false;
        Reporter::reportEvent( 1, "Using WAV File:", nextFileName);
        pSource = new WavSource(nextFileName,samples,bitsPerSample,channel);
    }else if(!strcasecmp(sourceType,"oss")){
        isdev = true;
        Reporter::reportEvent( 1, "Using OSS Device:", deviceID);
        pSource = new OssDspSource(deviceID,samples,bitsPerSample,channel);

    }else if(!strcasecmp(sourceType,"jack")){
        isdev = true;
        Reporter::reportEvent( 5, "Using ALSA Device:", deviceID);
        pSource = new JackDspSource(deviceID,epg_id,samples,bitsPerSample,channel);

    }

}

int
EpgSource::check_mysql_connect(MYSQL *mysqlptr)
{
    int retry = 5;
    char value = 1;
    int ret = -1;
    while(retry > 0) {
        if(!mysqlconnected) {
            if(!mysql_init(mysqlptr)) {
            
		 Reporter::reportEvent(1,"ERROR...","Mysql Init ERROR",mysql_error(mysqlptr));
                    retry--;
                    continue;
                }
            if(mysql_options(mysqlptr, MYSQL_OPT_RECONNECT, (char *)&value)) {
                printf("mysql_options error : %s\n", mysql_error(mysqlptr));
                Reporter::reportEvent(1,"ERROR...","Mysql Options ERROR",mysql_error(mysqlptr));  
                  retry--;
                    mysql_close(mysqlptr);
                    continue;
                }
            if(!mysql_real_connect(mysqlptr, dbhost, dbuser, dbpasswd,dbname, 0, 0, 0)) {
                //printf("mysql_real_connect error : %s\n", mysql_error(mysqlptr));              Reporter::reportEvent(1,"ERROR...","Mysql Connect ERROR",mysql_error(mysqlptr));
                retry--;
                mysql_close(mysqlptr);
                continue;
            }
            ret = 0;
            mysqlconnected = true;
            break;
        }else{
            if(mysql_ping(mysqlptr)){
                printf("mysql_ping error : %s\n", mysql_error(mysqlptr));
                mysql_close(mysqlptr);
                mysqlconnected = false;
                retry--;
            }else{
                ret = 0;
                break;
            }
        }
    }
    return ret;
}

int
EpgSource::prepareNextProgram(void)
{
    char strTime[9],curDate[11],curDoW[2];
    struct tm *ptr;
    time_t now;
    char query[1024];
    bool dflag, pflag;
    MYSQL_RES *result;
    MYSQL_ROW row;

    if(!started)
        now = time(NULL);
    else
        now = (time_t)(initTime.tv_sec + ((double)(initTime.tv_usec))/1000000 + playTime + 0.3);
    ptr = localtime(&now);
    memset(strTime, 0, sizeof(strTime));
    strftime(strTime,sizeof(strTime),"%T",ptr);
    strftime(curDate,sizeof(curDate),"%F",ptr);
    strftime(curDoW,sizeof(curDoW),"%u",ptr);
    Reporter::reportEvent(1,"Playing Start Time ",strTime);
    if(check_mysql_connect(&mysql)){
	Reporter::reportEvent(1,"ERROR....","Check_Mysql_Connect Failed");
        return -1;
    }
   // sprintf(query,"SELECT StartTime,EndTime,Elapse,SourceType,FilePath,FileName,Samples,BitsPerSample,Channel,DoW,SpecDate FROM %s WHERE FreqPath = '%s' AND ((StartTime <= '%s' AND EndTime > '%s') OR ((StartTime > EndTime) AND ((StartTime <= '%s') OR (EndTime > '%s'))))",dbtable,freqPath,strTime,strTime,strTime,strTime);
   sprintf(query,"SELECT EPGR_StartTime,EPGR_EndTime,SynTime,A_DDType,FilePath,FileNames,EPGR_IsAudioDev,EPGR_IsRecord,Result,EPGI_DoW,EPGI_SpecDate ,EPGI_SRVType EPGR_DeviceID FROM %s WHERE F_FID = '%s' AND ((EPGR_StartTime <= '%s' AND EPGR_EndTime > '%s') OR ((EPGR_StartTime > EPGR_EndTime) AND ((EPGR_StartTime <= '%s') OR (EPGR_EndTime > '%s'))))","V_SynFileInfoList",epg_id,strTime,strTime,strTime,strTime);
   // printf("sql = %s\n", query);
    if(mysql_query(&mysql,query)){
	    printf("mysql query error:%s\n", mysql_error(&mysql));
        return -1;
    }// need consider whether the connection to mysql is timeout, we should do some test to make sure the connection is ok.
	result =mysql_store_result(&mysql);
	if(result){
        dflag = false;
        pflag = false;
        int result_flag;
        //printf("halou ................/n");
        while(row = mysql_fetch_row(result)){
            Reporter::reportEvent(1,"Finding Program From DB");

            //printf("%s===%s===%s===%s===%s===%s===<%s>",row[4],row[5],row[6],row[7],row[8],row[9],row[10]);

            if(strncmp(row[2],curDate,10))
            {

                continue;

            }
          //  printf("strncmp row[2");
            if(row[8])
            {
                result_flag=atoi(row[8]);
                if(!result_flag)
                continue;

            }
          //  printf("dongadaeaeawed\n");
          //  printf("...%d......",strncmp(row[10],curDate,10));
            if(atoi(row[11])==1){
                //printf("row[10] is not null \n");
                if(strncmp(row[10],curDate,10)!=0)
                    continue;
                    dflag = true;
            printf("dong\n");
            }else if(row[9]!=NULL){
                if(!strstr(row[9],curDoW))
                    continue;
            }else
                continue;

			if(row[1])
                endTime = genTime(row[1], now);
            startTime = now;
			if((endTime > 0) && (startTime > endTime))
				endTime += 86400;
			reportEvent(3, "StartTime:",startTime," Endtime: ",endTime);
            elapse = (int)(endTime - startTime);
            if(row[12])
                sprintf(deviceID,"%s",row[12]);
            if(row[6])//isaudiodevice
                isDevice=atoi(row[6]);
			if(row[3])
			    sprintf(sourceType,"%s",row[3]);
            memset(filePath, 0, sizeof(filePath));
            memset(fileName, 0, sizeof(fileName));
            //make cfgfile
            if(row[4])
                sprintf(filePath,"%s",row[4]);
			if(row[5])
				sprintf(fileName,"%s",row[5]);
            if(!row[4] && !row[5])
                continue;
			/*if(row[6]){
	            sampleRate = samples = atoi(row[6]);
			}
			if(row[7])
    			bitsPerSample= atoi(row[7]);
			if(row[8])
				channel = atoi(row[8]);*/
            pflag = true;
            if(dflag)
                break;
        }
        if(!pflag){
             Reporter::reportEvent(1,"Warning......","Now No Program");
            //if(started)
              //  prepareBlankProgram();
            //else
                return -1;//return -1 prepareBlankProgram
               // prepareBlankProgram();
        }
        mysql_free_result(result);
    }else{
        printf("no program now\n");
        //if(started)
          //  prepareBlankProgram();
        //else
            return -1;//return -1 prepareBlankProgram
    }
    return 0;
}

void
EpgSource::prepareBlankProgram()
{
    printf("prepare play blank file\n");
    Reporter::reportEvent(1,"Warning....","Prepare Playing an Blank File Please Finding The Caulse");
    memset(filePath, 0, sizeof(filePath));
    memset(fileName, 0, sizeof(fileName));
    sprintf(nextFileName, "%s", blankFileName);
    sprintf(sourceType, "mp3");
    elapse = minTime;
}
int
EpgSource::setReadRange(int startTime,int endTime, int elapse){

    printf("FileStartTime=%d,  FileEndTime=%d,  elapse=%d\n",startTime,endTime,elapse);
    return pSource->setReadRange(startTime, endTime, elapse);
}

void
EpgSource::printTime(time_t t){
    struct tm *tblock;
    time_t now;
    now = t;
    tblock = localtime(&now);
    printf("%s",asctime(tblock));
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
   isOpened = true;
   lastFileEnd=true;// this is last adding when lastFileEnd is false when it init
//but i don't konwn why!
   Reporter::reportEvent(1,"Begin To Playing PlayList");
   return true;

}

bool
EpgSource:: canRead ( unsigned int    sec,
                      unsigned int    usec )    throw ( Exception )
{
    time_t now;
    
    if (!isOpen()){

        return false;
    }
 
  // printf("canread %d \n",lastFileEnd);
   
    if(lastFileEnd)
   {
      if(!isdev){

	Reporter::reportEvent(1,"Get SampleRate Channel BitsPerSample From File ");
         // printf("mp3_s:%d  mp3_channel:%d   mp3_samples %d",mp3_bitsPerSample,mp3_channel,mp3_samples);
          if(!mp3_bitsPerSample||!mp3_channel||!mp3_samples)
          {
              elapseLeft -= transBytesCur*1000/(bitsPerSample/8*channel*samples);
          }
          else
          {printf("transBytesCur  %d\n",transBytesCur);
          elapseLeft -= transBytesCur*1000/(mp3_bitsPerSample/8*mp3_channel*mp3_samples);
          }
        }
       else
        {
        elapseLeft -= transBytesCur*1000/(bitsPerSample/8*channel*samples);
       }
        Reporter::reportEvent(1,"Prepare Next Program Source");
        prepareNextSource();
        transBytesCur = 0;
        lastFileEnd = false;
        Reporter::reportEvent(1,"Now Begin Palying a New Program Name is :",nextFileName);
    }
    playTime = transBytesTotal/(bitsPerSample/8*channel*samples);
    now = time(NULL);
    if(playTime + initTime.tv_sec - now > 2)
	    usleep(50*1000);
    return pSource->canRead(sec,usec);
}

unsigned int
EpgSource :: read ( void          * buf,
                    unsigned int    len )     throw ( Exception )
{

    ssize_t ret;

    if( !isOpen() ){
        return 0;
    }
    do{
        ret = pSource->read(buf, len);
        transBytesTotal += ret;
        transBytesCur += ret;
        count++;
        if(!isdev && ret == 0){//check whether end for file
            lastFileEnd = true;
            Reporter::reportEvent(1,"This File Play End");
            pSource->strip();
           // printf("File %s read over    count = %d  time : ", nextFileName, count);
            //printTime(time(NULL));
            //printf("\n");
            Reporter::reportEvent(1,"File",nextFileName,"Play End  ");
            canRead(1, 0);
        }else if(isdev && elapseLeft<= transBytesCur*1000/(bitsPerSample/8*channel*samples)){//check whether end for device
            lastFileEnd = true;
            pSource->strip();
            Reporter::reportEvent(1,"Device Play End");
            return ret;
        }
    }while(ret == 0);

    return ret;
}


void
EpgSource :: close ( void )                  throw ( Exception )
{
    if ( !isOpen() ){
        return;
    }
    pSource->close();
    delete pSource;
    pSource = NULL;
    printf("Epg closed\n");

}
