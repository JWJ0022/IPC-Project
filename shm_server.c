#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#define SHMKEY1 (key_t)60031
#define BILLION 1000000000L

void readMessage(void* arg);
void decode(void);
void printDecode(void* arg);

struct threadInfo
{
	sem_t* sem1;
	sem_t* sem2;
	char* shmaddr;
};

char readfromshm[100];
char decodetext[100];

sem_t a;
sem_t b;
sem_t c;
sem_t d;

int main(void)
{
	pthread_t thread1,thread2,thread3;
	pid_t pid1;
	pid_t pid2;
	int shmid1, shmid2;
	char* shmaddr1, shmaddr2;
	sem_t* sem1;
	sem_t* sem2;
	sem_t* sem3;
	sem_t* sem4;
	struct threadInfo child1info;
	struct threadInfo child2info;

	printf("[Parent]1번 공유 메모리 생성\n");
	shmid1 = shmget(SHMKEY1, 20, IPC_CREAT|0666);//공유 메모리 생성 및 접근
        shmaddr1 = (char*)shmat(shmid1, 0, 0); //프로세스의 논리 주소 공간에 부착

	sem1 = sem_open("/my_sem1", O_CREAT, 0666, 0); //named 세마포어 생성
	sem2 = sem_open("/my_sem2", O_CREAT, 0666, 0); //named 세마포어 생성
	sem3 = sem_open("/my_sem3", O_CREAT, 0666, 0);
	sem4 = sem_open("/my_sem4", O_CREAT, 0666, 0);

	child1info.sem1 = sem1;
	child1info.sem2 = sem2;
	child1info.shmaddr = shmaddr1;

	child2info.sem1 = sem3;
	child2info.sem2 = sem4;
	child2info.shmaddr = shmaddr1;

	sem_init(&a, 0, 1);
	sem_init(&b, 0, 0);
	sem_init(&c, 0, 1);
	sem_init(&d, 0, 0);

	printf("[Parent]자식 프로세스 1 fork()\n");
	if((pid1=fork())<0)//1번 자식 프로세스 생성
	{
		perror("[Child1]fork error\n");
		exit(1);
	} else if(pid1 == 0)//1번 자식 프로세스 영역
	{
		struct timespec start;
		struct timespec stop;

		printf("[Child1]1번 자식 프로세스 생성\n");

		//자식프로세스 영역
		if(pthread_create(&thread1, NULL, readMessage, (void*)&child1info))
		{
			perror("[Child1]쓰레드1 생성 실패\n");
			exit(1);
		}
		if(pthread_create(&thread2, NULL, decode, NULL))
		{
			perror("[Clild1]쓰레드2 생성 실패\n");
			exit(1);
		}
		if(pthread_create(&thread3, NULL, printDecode, (void*)&start))
		{
			perror("[Child1]쓰레드3 생성 실패\n");
			exit(1);
		}
		
		pthread_join(thread1, NULL);
		printf("[Child1]thread1 종료\n");
		pthread_join(thread2, NULL);
		printf("[Child1]thread2 종료\n");
		pthread_join(thread3, NULL);
		printf("[Child1]thread3 종료\n");

		clock_gettime(CLOCK_MONOTONIC, &stop);

		sem_unlink("/my_sem1");
		printf("[Child1]/my_sem1 세마포어 unlink\n");

		sem_unlink("/my_sem2");
		printf("[Child1]/my_sem2 세마포어 unlink\n");

		printf("[Child1]걸린 시간 : %.9f seconds\n", (stop.tv_sec - start.tv_sec)+(double)(stop.tv_nsec - start.tv_nsec)/(double)BILLION);

		printf("[Child1]종료\n");
		exit(0);
		printf("[Child1]종료되지 않았다면 출력\n");
	}
	
	printf("[Parent]자식 프로세스2 fork()\n");
	if((pid2=fork())<0)//2번 자식 프로세스 생성
        {
                perror("[Child2]fork error\n");
                exit(1);
        } else if(pid2 == 0)//2번 자식 프로세스 영역
        {
		struct timespec start;
		struct timespec stop;

		printf("[Child2]2번 자식 프로세스 생성\n"); 
                
                if(pthread_create(&thread1, NULL, readMessage, (void*)&child2info))
                {
                        perror("[Child2]쓰레드1 생성실패\n");
                        exit(1);
                }
                if(pthread_create(&thread2, NULL, decode, NULL))
                {
                        perror("[Child2]쓰레드2 생성 실패\n");
                        exit(1);
                }
                if(pthread_create(&thread3, NULL, printDecode, (void*)&start))
                {
                        perror("[Child2] 쓰레드3 생성 실패\n");
                        exit(1);
                }

                pthread_join(thread1, NULL);
                printf("[Child2]thread1 종료\n");
                pthread_join(thread2, NULL);
                printf("[Child2]thread2 종료\n");
                pthread_join(thread3, NULL);
                printf("[Child2]thread3 종료\n");

                clock_gettime(CLOCK_MONOTONIC, &stop);

		sem_unlink("/my_sem3");
		printf("[Child2]/my_sem3 세마포어 unlink\n");

		sem_unlink("my_sem4");
		printf("[Child2]/my_sem4 세마포어 unlink\n");

		printf("[Child2]걸린 시간 : %.9f seconds\n", (stop.tv_sec - start.tv_sec)+(double)(stop.tv_nsec - start.tv_nsec)/(double)BILLION);

                exit(0);
	}

	if(pid1>0 && pid2>0)//부모 프로세스의 영역
	{
		printf("[Parent] 1번 자식 프로세스의 종료를 wait\n");
		waitpid(pid1,NULL,0); //1번 자식 프로세스의 종료를 기다림
		printf("[Parnet] 1번 자식 프로세스 종료\n");
		

		printf("[Parent] 2번 자식 프로세스 종료를 wait\n");
		waitpid(pid2,NULL,0); //2번 자식 프로세스의 종료를 기다림
		printf("[Parent] 2번 자식 프로세스 종료\n");

		shmdt(shmaddr1);
		shmctl(shmid1, IPC_RMID, NULL);	
	}
	
	printf("[Parent]부모 프로세스 종료\n");
	exit(0);
}

void readMessage(void* arg)
{
	int shmid;
	char* shmaddr;
	sem_t* sem1;
	sem_t* sem2;
	struct threadInfo* info = (struct threadInfo*)arg;
	char end[]="EOF";

	sem1 = info -> sem1;
	sem2 = info -> sem2;
	shmaddr = info -> shmaddr;	

	sem_post(sem1);
	while(1)
	{
		sem_wait(sem2);

		sem_wait(&a);
		
		memcpy(readfromshm, shmaddr, 99);//공유 메모리에서 읽어서 readfromshm배열에 저장

		if(strcmp(readfromshm,"EOF")==0)
		{
			memset(shmaddr,0,100); //공유 메모리 비움
			sem_post(sem1);
			sem_post(&b);

			break;
		}

		printf("[thread1]readSharedMemory : 공유 메모리에서 데이터 읽음 -> %s\n", readfromshm);
		
		memset(shmaddr, 0, 100); //공유 메모리 비움

		sem_post(&b);

		sem_post(sem1);
	}

	pthread_exit(NULL);
}

void decode(void)
{

	while(1)
	{
		sem_wait(&b);

		sem_wait(&c);

		if(strcmp(readfromshm,"EOF")==0)
		{	
			strcpy(decodetext, "EOF");

			sem_post(&a);
			sem_post(&d);

			break;
		}

		for(int i=0; i<strlen(readfromshm); i++)
		{
			decodetext[i]=readfromshm[i] + 1; //readfromshm에 저장된 암호화된 텍스트를 복호화해서 decodetext에 저장
		}

		strcpy(readfromshm,""); //readfromshm 배열을 비워줌

		sem_post(&a);

		sem_post(&d);
	}

	pthread_exit(NULL);
}

void printDecode(void* arg)
{
	struct timespec* start = (struct timespec*)arg;
	int timestart=0;
	while(1)
	{
		sem_wait(&d);

		if(timestart == 0) //시작하는 순간에 시간 측정
		{
			clock_gettime(CLOCK_MONOTONIC, start);
			timestart=1;//처음 시작하는 시점에만 시작 시간 측정
		}

		if(strcmp(decodetext,"EOF")==0)
		{
			sem_post(&c);

			break;
		}

		printf("[thread3]print : 복호화 결과 : %s\n", decodetext);

		strcpy(decodetext, ""); //decodetext 배열 비워줌

		sem_post(&c);
	}

	pthread_exit(NULL);
}
