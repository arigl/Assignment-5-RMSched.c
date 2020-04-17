/*
	Name: Alex Rigl
	ID: 2300146
	Due date: 4/15/2020
	Assignment: 5

	Partner: Filip Augustkowski
	
	Resources
	- Tristan Chilvers helped us a lot with certain functions of the program, really appreciate his help

	Description:
	- RMsched.c
*/

//----VARIABLES AND PACKAGES-----------------------------------------------
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <semaphore.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

/* Structs */
typedef struct{
  char* name;
  int wcet;
  int period;
  int curr;
  int counter;
}Task;

typedef struct{
  Task *tasks;
  int numTasks;
}AllTaskInfo;

/* Task Constructor */
void NewTask(Task* taskPTR, char* name, int wcet, int period);

/* Holds information on all of our executing threads */
AllTaskInfo taskInfo;

/* Methods */
void *Simulator();
void *newSimulator();
void *SetupSimulation();
int lineSize = 255;
int reqArgs = 4;
int lcm;
char str[32];
int ticks = 0;
int curr = 0;

/* Arrays */
int *scheduleTimes;
int *scheduleTasks;

FILE *taskSetFile;
FILE *scheduleFile;

void *TaskThread(void *param); /* Thread function */
sem_t *simLock; /* Main semaphore */
sem_t *myLock; /* Lock specific to current thread executing */
pthread_t *tid; /* Process Thread array */



//----MAIN------------------------------------------------------------------
int main(int argc, char** argv)
{
  /* Allocate memory to our array pointers */

  scheduleTimes = malloc(lineSize * sizeof scheduleTimes);
  scheduleTasks = malloc(lineSize * sizeof scheduleTasks);
  /* First Get Input and Check Each one */
  if (CheckInput(argc,argv)) {
    //Try to parse through files to get task set
    SetupSimulation();
  }

}


//----HELPER METHODS--------------------------------------------------------
/* Method that checks input from command line */
int CheckInput(int argc, char** argv)
{
    if (argc == reqArgs) { /*Check number of args */


      /* Fetch file names and try to open them */
      taskSetFile = fopen(argv[2], "r");
      scheduleFile = fopen(argv[3], "r+");

      if (atoi(argv[1]) <= 0) { /* Check number of entered periods */
        printf("ERROR: N periods must be greater than 0 \n");
        exit(EXIT_FAILURE);
      }
      /* Check if files are opened */
      else if(scheduleFile == NULL) {
        printf("ERROR: Schedule file cannot be opened \n");
        exit(EXIT_FAILURE);
      }
      else if(taskSetFile == NULL) {
        printf("ERROR: Task Set file cannot be opened \n");
        exit(EXIT_FAILURE);
      }
      else { /* Input is correct */
        /* Now one can read information from files */
        ReadFiles(taskSetFile);
        return 1;
      }
    }
    else {
      printf("ERROR: Not enough arguments \n");
      exit(EXIT_FAILURE);
    }
}

int ReadFiles(FILE *taskFile) {
    char fileLine[lineSize], *taskName, c;
    taskName = malloc(2 * sizeof(taskName));
    int wcet, period, temp, i = 0;

    taskInfo.tasks = malloc(lineSize * sizeof(taskInfo.tasks));


    /* Get information from each line and put into tasks */
    while (fgets(fileLine, lineSize, taskFile) != NULL) {
      sscanf(fileLine, "%s%d%d", taskName, &wcet, &period); /* scan each string/line for info */
      // parse wcet and period as int


      Task* newTask = malloc(sizeof(Task));
      NewTask(newTask, taskName, wcet, period);


      taskInfo.tasks[i] = *newTask;

      i++;
    }

    /* i counted the amount threads */
    taskInfo.numTasks = i;

    fclose(taskFile);

}



void NewTask(Task* taskPTR, char* name, int wcet, int period) {

  taskPTR -> name = strdup(name);
  taskPTR -> wcet = wcet;
  taskPTR -> period = period;
  taskPTR -> curr = 0;
  taskPTR -> counter = 0;

}

void *SetupSimulation() {
  int i;


  /* Initialize all Semaphores */
  simLock = malloc(sizeof(sem_t));
  myLock = malloc((taskInfo.numTasks) * sizeof(sem_t));

  /* Breaks if it doesnt work */
  if(CheckThread(sem_init(&simLock, 0, 0))) {
    for (i = 0; i < taskInfo.numTasks; i++) {
      if(CheckThread(sem_init(&myLock[i], 0, 0))) {
        continue;
      }
    }
  }

  /* Initialize all Threads and attributes */
  tid = malloc(sizeof(pthread_t) * taskInfo.numTasks);


  for (i = 0; i < taskInfo.numTasks; i++) {
    int *taskNum = (int *)malloc(sizeof(int));
    *taskNum = i;

    if (CheckThread(pthread_create(&tid[i], NULL, TaskThread, (void *)taskNum))) {
	continue;
    }
  }

  //printf("Right before calling Simulator \n");
  //Simulator();
  newSimulator();
  fflush(stdout);

  for (i = 0; i < taskInfo.numTasks; i++){
	sem_post(&myLock[i]);
  }

  /* Join all threads */
  for (i = 0; i < taskInfo.numTasks; i++) {
    pthread_join(tid[i], NULL);
  }


  free(myLock);
  //free(taskInfo);
  return 0;
}

void *newSimulator() {
  lcm = findLCM();
  bubbleSort(taskInfo.numTasks);

  int i = 0;
  int j = 0;
  int k = 0;

  for (i = 0; i < lcm; i++)
  {
	if (i>9)
	{
		sprintf(str, "%d ", i);
                fflush(scheduleFile);
                fprintf(scheduleFile, "%s", &str);
	}
	else{
		sprintf(str, "%d ", i);
                fflush(scheduleFile);
		fprintf(scheduleFile, "%s", &str);
	}
  }

  //printf("First for loop done \n");
  fflush(scheduleFile);
  fprintf(scheduleFile, "\n");

  int cont = 1;

  while(ticks != lcm){
	for (i = 0; i < taskInfo.numTasks; i++){
		for (j = taskInfo.tasks[i].curr; j < taskInfo.tasks[i].wcet; j++){
			sprintf(str, "%s ", taskInfo.tasks[i].name);
			fprintf(scheduleFile, "%s", taskInfo.tasks[i].name);
			fflush(stdout);
			ticks++;
			taskInfo.tasks[i].curr++;
			for(k = 0; k < taskInfo.numTasks; k++){
				taskInfo.tasks[k].counter++;
				//printf("Task %s counter is %d time: %d\n", taskInfo.tasks[k].name,      taskInfo.tasks[k].counter,ticks);
				if(taskInfo.tasks[k].counter == taskInfo.tasks[k].period){
					taskInfo.tasks[k].curr = 0;
					taskInfo.tasks[k].counter = 0;
					i = -1;
					cont = 0;
				}
			}
			if(cont == 0){
				cont = 1;
				break;
			}
		}
	}
	for (k = 0; k < taskInfo.numTasks; k++)
	{
		taskInfo.tasks[i].counter++;
		if(taskInfo.tasks[k].counter == taskInfo.tasks[k].period)
		{

			taskInfo.tasks[k].curr = 0;
			taskInfo.tasks[k].counter = 0;
		}
	}
        fflush(scheduleFile);
	fprintf(scheduleFile, "%s", str);
	fflush(stdout);
	ticks++;

  }
  fflush(scheduleFile);
  fprintf(scheduleFile, "%s", str);
  curr = -1;
}

int CheckThread(int checkInt)
{
    if (checkInt != 0) {
      perror("Initialization error \n");
      exit(EXIT_FAILURE);
    }
    else
    {
      return 1;
    }
}

void *TaskThread(void *param)
{

  int me = *((int *) param);
  printf("Thread speaking \n");


  while(curr != -1) {
	if(sem_wait(&myLock[me] != 0)){
		printf("Semaphore error in thread\n");
		exit(EXIT_FAILURE);
	}

	if(curr == -1){
		break;
	}
	printf("This is the %d thread speaking!\n\n", me);
	fflush(stdout);
	sem_post(simLock);
  }
  free(param);
  pthread_exit(0);

}
int gcd(int a, int b)
{
	if (a == 0)
		return b;
	return gcd(b % a, a);
}
int findLCM()
{
	int ans = taskInfo.tasks[0].period;
	int i = 1;
	for (i = 1; i < taskInfo.numTasks; i++){
		ans = (((taskInfo.tasks[i].period *ans)) / (gcd(taskInfo.tasks[i].period, ans)));
	}
	return ans;
}

void bubbleSort(int n)
{
	int i = 0;
	if (n == 1){
		return;
	}
	for (i = 0; i < n - 1; i++){
		if(taskInfo.tasks[i].period > taskInfo.tasks[i+1].period){
			Task tempTask = taskInfo.tasks[i];
			taskInfo.tasks[i] = taskInfo.tasks[i+1];
			taskInfo.tasks[i+1] = tempTask;
		}
	}
}
