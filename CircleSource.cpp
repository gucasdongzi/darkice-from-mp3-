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

#include <malloc.h>
#include <limits.h>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include "CircleSource.h"
#include "Mp3FileSource.h"
#include "DarkIceConfig.h"
int CircleSource :: file_num(char * dir)
{
        DIR* dp;
        int fileNum = 0;
        struct dirent *entry;
        struct stat statbuf;
        if((dp = opendir(dir)) == NULL){
                printf("cannot open %s\n",dir);
                return 0;
        }
        chdir(dir);
        while ((entry = readdir(dp)) != NULL){
                lstat(entry->d_name,&statbuf);
                if(S_ISREG(statbuf.st_mode))
                        fileNum++;
        }
        closedir(dp);
        return fileNum;

}

void CircleSource :: print_seq(int *a, int len)
{
        int i;
        printf("The randSeq is:");
        for(i = 0; i < len-1; i++)
                printf(" %d",a[i]);
        printf("\n");
}


void CircleSource :: select_n_from_old(char *oldDir, int oldFileNum, char playFileName[][255], int selectNum)
{
        int randNum,i,j,k,*randSeq,seqLen=1;
        DIR* dp;
        struct dirent *entry;
        struct stat statbuf;
        srand((unsigned)time(NULL));
        randSeq = (int *)malloc((selectNum+1) * sizeof(int));
        randSeq[0] = INT_MAX;
        for(i = 0; i < selectNum; i++){
                randNum = rand() % (oldFileNum - i);
                j = 0;
                while(randNum >= randSeq[j]){
                        j++;
                        randNum++;
                }
                seqLen++;
                for(k = seqLen -1; k >= j+1; k--)
                        randSeq[k] = randSeq[k-1];
                randSeq[j] = randNum;
        }
        print_seq(randSeq,seqLen);
        chdir(oldDir);
        dp = opendir(oldDir);
        i = 0;//the file ID
        j = 0;//the randSeq ID
        while((entry = readdir(dp)) != NULL){
                lstat(entry->d_name,&statbuf);
                if(!S_ISREG(statbuf.st_mode))
                        continue;
                if(i != randSeq[j]){
                        i++;
                        continue;
                }
                sprintf(playFileName[j],"%s/%s",oldDir,entry->d_name);
                //printf("%s    file %d: %s\n",entry->d_name,j,playListName[j]);
                i++;
                j++;
                if(j >= seqLen -1)
                        break;
        }
        closedir(dp);
        free(randSeq);
}
void CircleSource :: select_and_move_one_from_new_to_old(char *oldDir, int oldFileNum, char *newDir, int newFileNum, char *playFile)
{
        int randNum,i,j;
        DIR* dp;
        struct dirent *entry;
        struct stat statbuf;
        char oldname[255],newname[255];
        srand((unsigned)time(NULL));
        randNum = rand() % newFileNum;
        dp = opendir(newDir);
        chdir(newDir);
        i = 0;
        while((entry = readdir(dp)) != NULL){
                lstat(entry->d_name,&statbuf);
                if(!S_ISREG(statbuf.st_mode))
                        continue;
                if(i == randNum)
                        break;
                i++;
        }
        sprintf(oldname,"%s/%s",newDir,entry->d_name);
        sprintf(newname,"%s/%s",oldDir,entry->d_name);
        printf("oldname=%s      newname=%s\n",oldname,newname);
        j = rename(oldname,newname);
        printf("rename result = %d\n",j);
        strcpy(playFile,newname);
        closedir(dp);
}

void CircleSource :: select_one_from_dir(char* curDir, int dirFileNum, char* playFile)
{
	int randNum,i;
	DIR* dp;
	struct dirent *entry;
	struct stat statbuf;
	srand((unsigned)time(NULL));
	randNum = rand() % dirFileNum;
	dp = opendir(curDir);
	chdir(curDir);
	i = 0;
	while((entry = readdir(dp)) != NULL){
		lstat(entry->d_name,&statbuf);
		if(!S_ISREG(statbuf.st_mode))
			continue;
		if(i == randNum)
			break;
		i++;
	}
	sprintf(playFile,"%s/%s",curDir,entry->d_name);
	//printf("select file is %s\n",playFile);
	closedir(dp);
}	

int CircleSource :: make_playlist(void)
{
        int oldFileNum,newFileNum,oldPlayNum,newPlayNum,i;
        oldFileNum = file_num(oldDir);//fileNum in old dir
        newFileNum = file_num(newDir);//fileNum in new dir
        printf("oldFileNum = %d\n",oldFileNum);
        printf("newFileNum = %d\n",newFileNum);
        if(oldFileNum + newFileNum < playNum){
                printf("not enough file\n");
                return 0;
        }

        if(oldFileNum < oldNum){
                oldPlayNum = oldFileNum;
                newPlayNum = playNum - oldPlayNum;
        }
        else if (newFileNum < newNum){
                newPlayNum = newFileNum;
                oldPlayNum = playNum - newPlayNum;
        }
        else{
                oldPlayNum = oldNum;
                newPlayNum = newNum;
        }
        printf("oldPlayNum = %d\n",oldPlayNum);
        printf("newPlayNum = %d\n",newPlayNum);
        select_n_from_old(oldDir, oldFileNum, playList, oldPlayNum);
        for(i = 0; i < newPlayNum; i++)
                select_and_move_one_from_new_to_old(oldDir, oldFileNum++, newDir, newFileNum--,playList[i+oldPlayNum]);
        for(i = 0; i < playNum; i++)
                printf("%s\n",playList[i]);
        return 0;
}



bool
CircleSource :: isBigEndian ( void ) const                  throw ()
{
      return false;
}


void
CircleSource :: init (  const char      * name )    throw ( Exception )
{    

	std::ifstream       configFile(cfgfile);
	Config              config( configFile);
	const ConfigSection    * cs;
       	const char             * str;


	isOpened=false;	   
        pSource =NULL;
	count = 0;
	musicTurn = false;
	firstCanRead = true;
	lastFileEnd = true;
	lastListTimes = listPlayNum;
	nextInList = 0;
	nextPlayTime = 0;
	nextPlayTimeMs = 0;
	days = 0;
	transBytes = 0;
	playTime = 0;
	setvbuf(stdout, NULL, _IONBF, 0);
	if ( !(cs = config.get( "general")) ) {
                     throw Exception( __FILE__, __LINE__, "no section [general] in config");
        }

        str = cs->getForSure( "oldfiledir", " missing in section [general]");

	sprintf(oldDir,"%s",str);

	str = cs->getForSure("newfiledir","missing in section [general]");

	sprintf(newDir,"%s",str);

	str = cs->getForSure("musicfiledir","missing in section [general]");

	sprintf(musicDir,"%s",str);
	
	printf("%s\n%s\n%s\n",oldDir,newDir,musicDir);

	
}
int CircleSource::genTime(char *strtime){

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


void CircleSource::printTime(time_t t){
	struct tm *tblock;
	time_t now;
	now = t;
	tblock = localtime(&now);
	printf("%s",asctime(tblock));
}


void
CircleSource::prepareNextSource(void){
	Reporter::reportEvent( 1, "Using Mp3 File:", fileName);
        pSource =  new Mp3FileSource(fileName,samples,bitsPerSample,channel); 

}


int 
CircleSource::setReadRange(int startTime,int endTime, int elapse){

    //printf("FileStartTime=%d,  FileEndTime=%d,  elapse=%d\n",startTime,endTime,elapse);
    return pSource->setReadRange(startTime,endTime,elapse);
}

void
CircleSource :: strip ( void )                      throw ( Exception )
{
    if ( isOpen() ) {
        close();
    }
    
}

bool
CircleSource :: open ( void )                       throw ( Exception ){

    if ( isOpen() ) {
        return false;
    }
   	isOpened=true; 
	printf("circle open\n");
   	return true;	

}

bool
CircleSource:: canRead ( unsigned int    sec,
                          unsigned int    usec )    throw ( Exception )
{

	if ( !isOpen() ) {
        	return false;
    }
	if (firstCanRead) {
		initTime = time(NULL);
		firstCanRead = false;
	}
	//play new file
    if (lastFileEnd) {
		//play music
		if (musicTurn) {
			select_one_from_dir(musicDir, file_num(musicDir), musicName);
			strcpy(fileName,musicName);
		}
		//play file in list
		else {

			//play the 1st file of the list	
			if (nextInList == 0) {
				//play new list
				if (lastListTimes == listPlayNum) {
					make_playlist();
					lastListTimes = 1;
					days++;
					printf("\n\n");
					printf("It is the %dth day\n",days);
				}
				else {
					lastListTimes++;
				}
				printf("It is the %dth times to play this list\n\n",lastListTimes);
			}
			strcpy(fileName,playList[nextInList]);
			nextInList = (nextInList + 1) % playNum;
		}
		/*
		for(int i=0;i<playNum;i++)
			printf("the %dth file in the playList is: %s\n",i+1,playList[i]);
		*/
  		prepareNextSource();
		pSource->open();
		setReadRange(0,0,0);
		printf("fileTime = %d S\n",pSource->fileTime/1000);
		nextPlayTimeMs += pSource->fileTime;
		lastFileEnd = false;
		curTime = time(NULL);
		printf("Local time is: ");
		printTime(time(NULL));
		/*
		nextPlayTime = initTime + (time_t)(nextPlayTimeMs/1000);
		printf("Next file will be played at: ");
		printTime(nextPlayTime);
		*/
		musicTurn = !musicTurn;
		count = 0;
		
	}	
	//playTime = transBytes/(1152*2*2)*pSource->frameTime/1000;
    playTime = transBytes/(bitsPerSample/8*channel*samples);
	now = time(NULL);
	if (playTime+initTime-now > 2)
		usleep(30*1000);
	//else usleep(15*1000);
    	return pSource->canRead(sec,usec);
}

unsigned int
CircleSource :: read (    void          * buf,
                          unsigned int    len )     throw ( Exception )
{

	ssize_t         ret;
    if ( !isOpen() ) {
        return 0;
    }
    do{
    	ret =  pSource->read(buf,len);
	    count++;
	    //printf("count == %d\n",count);
	    //printf("count = %d	ret = %d	endFlag = %d\n",count,ret,pSource->endFlag);
	    if (ret == 0) {
		    lastFileEnd = true;		
		    printf("now fileEnd\n");
		    pSource->strip();
		    printf("%s read over		count = %d	time : ",fileName,count);
		    printTime(time(NULL));
		    printf("\n");
		    canRead(0,0);
	    }
    } while (ret == 0);
	transBytes += ret;
   	return ret;

}



void
CircleSource :: close ( void )                  throw ( Exception )
{
    if ( !isOpen() ) {
        return;
    }
    pSource->close();
    delete pSource;
    pSource = NULL;
    printf("Circle closed\n");

}


