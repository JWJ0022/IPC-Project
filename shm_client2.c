#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>

#define SHMKEY (key_t)60031

void readFile(void);
void checkFile(void);
void sendFile(void);

char NewsText[100]; //.txt 파일에서 읽은 내용을 일차적으로 저장하는 배열
char CheckedText[100]; //비속어 삭제 처리를 한 결과를 저장하는 배열
sem_t a;
sem_t b;
sem_t c;
sem_t d;

int main()
{
	pthread_t thread1, thread2, thread3;
	int err;

	sem_init(&a, 0, 1);//unnamed 세마포어 초기화
	sem_init(&b, 0, 0);//unnamed 세마포어 초기화
	sem_init(&c, 0, 1);//unnamed 세마포어 초기화
	sem_init(&d, 0, 0);//unnamed 세마포어 초기화	

	err = pthread_create(&thread1, NULL, readFile, NULL);//thread1에 readFile 함수 할당
	if(err != 0)
	{
		printf("ERROR code is %d\n", err);
		pthread_exit(NULL);
	}
	
	err = pthread_create(&thread2, NULL, checkFile, NULL);//thread2에 checkFile 함수 할당
	if(err != 0)
	{
		printf("ERROR code is %d\n", err);
		pthread_exit(1);
	}

	err = pthread_create(&thread3, NULL, sendFile, NULL); //thread3에 sharedMemory 함수 할당,인자로 name 세마포어의 주소 전달
	if(err != 0)
	{
		printf("ERROR code is %d\n", err);
		pthread_exit(1);
	}

	pthread_exit(NULL);
		
}

void readFile(void)//.txt 파일에서 한 문장씩 읽어서 NewsText 전역 배열에 저장
{
	FILE* fp;
	int ch;
	char* res;

	fp = fopen("b.txt", "r"); //file open
	if(fp == NULL)
	{
		printf("[client]ReadFile : 파일이 열리지 않았습니다.\n");
		pthread_exit(1);
	}
	printf("[client]ReadFile : 파일이 열렸습니다.\n");
	
	while(1)
	{
		sem_wait(&a);
		
		res = fgets(NewsText, sizeof(NewsText), fp); //.txt 파일에서 한문장씩 읽어서 NewsText 배열에 저장
		if(res == NULL) //문장을 다 읽으면 fgets 함수가 NULL을 반환
		{
			strcpy(NewsText, "EOF");

			sem_post(&b);

			break; //반복문 탈출
		}

		NewsText[strlen(NewsText) - 1] = '\0'; //문장의 끝에 개행문자를 제거하고  종료 문자 설정
		printf("[client]ReadFile : %s\n", NewsText);
		
		sem_post(&b);
	}

	fclose(fp);

	pthread_exit(NULL);
}

void checkFile(void)
{

	while(1)
	{
		sem_wait(&b);

		sem_wait(&c);
		
		if(strcmp(NewsText,"EOF")==0)
		{
			printf("[client]CheckFile 쓰레드 종료\n");
			strcpy(CheckedText, "EOF");

			sem_post(&a);
			sem_post(&d);

			break;
		}

		for(int i=0; i<strlen(NewsText); i++)
		{
			CheckedText[i] = NewsText[i] - 1; //NewsText에 저장된 텍스트를 암호화해서 CheckedText 배열에 저장
		}

		CheckedText[strlen(CheckedText)] = "\0"; //CheckedText에 종료 문자 삽입

		printf("[client]CheckFile : CheckedText 배열 출력 -> %s\n", CheckedText);
		strcpy(NewsText,""); //NewsText 배열 비움

                sem_post(&d);

                sem_post(&a);
	}
 
	pthread_exit(NULL);
}

void sendFile(void)
{
	char* shmaddr;

        sem_t *sem1 = sem_open("/my_sem3", 0); //named세마포어 open
        if(sem1 == SEM_FAILED)//예외 처리
        {
                perror("sem_open error");
                pthread_exit(1);
        }
	sem_t *sem2 = sem_open("/my_sem4", 0); //named세마포어 open
        if(sem1 == SEM_FAILED)//예외 처리
        {
                perror("sem_open error");
                pthread_exit(1);
        }

	int shmid = shmget(SHMKEY, 0, 0);//서버가 생성한 공유 메모리에 접근하기 위해 size인수와 mode 인수를 0으로 설정
	if(shmid == -1)
	{
		printf("[client]sharedMemory : shmget 오류\n");
		pthread_exit(NULL);
	}

	shmaddr = (char*)shmat(shmid, 0, 0);//공유 메모리 구역을 프로세스의 논리 주소 공간에 부착

	while(1)
	{
		sem_wait(&d);

		sem_wait(sem1);
		
		if(strcmp(CheckedText,"EOF")==0)
		{
			memcpy(shmaddr, CheckedText, strlen(CheckedText));

			sem_post(sem2);
			printf("[client]공유 메모리에 쓰기 끝\n");
			sem_post(&c);
 			
			break;
		}

		memcpy(shmaddr, CheckedText, strlen(CheckedText));
		printf("[clinet]sharedMemory : 공유 메모리 영역에 %s 쓰기 완료\n", CheckedText);
		strcpy(CheckedText, ""); //CheckedText 배열 비움

                sem_post(&c);

		sem_post(sem2);
	}

	pthread_exit(NULL);
}
