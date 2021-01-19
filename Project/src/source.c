/*
 * File:	a3-partial.c
 * Author:	Andrew Merer
 * Date:	2020-06-17
 * Version:	1.0
 *
 * Purpose:	To simulate various scheduling algrithms
 */


#include    <stdio.h>
#include    <stdlib.h>
#include    <stdbool.h>
#include    <string.h>
#include	<time.h>
#include	<math.h>

#ifdef DEBUG
#define D_PRNT(...) fprintf(stderr, __VA_ARGS__)
#else
#define D_PRNT(...)
#endif

#define     DEFAULT_INIT_JOBS	    5
#define     DEFAULT_TOTAL_JOBS	    100
#define     DEFAULT_LAMBDA	    ((double)1.0)
#define     DEFAULT_SCHED_TIME	    10		    // usec
#define     DEFAULT_CONT_SWTCH_TIME 50		    // usec
#define     DEFAULT_TICK_TIME	    10		    // msec
#define     DEFAULT_PROB_NEW_JOB    ((double)0.15)
#define     DEFAULT_RANDOMIZE	    false

enum sched_alg_T { UNDEFINED, RR, SJF, FCFS };
char * alg_names[] = { "UNDEFINED", "RR", "SJF", "FCFS" };
/*Input parameters*/
struct simulation_params
{
    enum sched_alg_T sched_alg;
    int init_jobs;
    int total_jobs;
    double lambda;
    int sched_time;
    int cont_swtch_time;
    int tick_time;
    double prob_new_job;
    bool randomize;
};
/*struct for array of customers*/
struct customer
{ 
    int arrival_time;
    int first_serve_time;
    int depart_time;
    int burst_time;
};

struct customer *array_of_customers;
unsigned long long int bottom = 0;
unsigned long long int top = 0;
unsigned long long int global_time = 0;
char * progname;

void usage(const char * message)
{
    fprintf(stderr, "%s", message);
    fprintf(stderr, "Usage: %s <arguments>\nwhere the arguments are:\n",
	    progname);
    fprintf(stderr, "\t-alg [rr|sjf|fcfs]\n"
	    "\t[-init_jobs <n1 (int)>\n"
	    "\t[-total_jobs <n2 (int)>]\n"
	    "\t[-prob_comp_time <lambda (double)>]\n"
	    "\t[-sched_time <ts (int, microseconds)>]\n"
	    "\t[-cs_time <cs (int, microseconds)>]\n"
	    "\t[-tick_time <cs (int, milliseconds)>]\n"
	    "\t[-prob_new_job <pnj (double)>]\n"
	    "\t[-randomize]\n");
}



/*
 * Name:	process_args()
 * Purpose:	Process the arguments, checking for validity where possible.
 * Arguments:	argc, argv and a struct to hold the values in.
 * Outputs:	Error messages only.
 * Modifies:	The struct argument.
 * Returns:	0 on success, non-0 otherwise.
 * Assumptions:	The pointers are valid.
 * Bugs:	This is way too long.  Maybe I should reconsider how
 *		much I dislike getopt().
 * Notes:	Sets the struct to the default values before
 *		processing the args.
 */

int process_args(int argc, char * argv[], struct simulation_params * sps)
{
    // Process the command-line arguments.
    // The only one which doesn't have a default (and thus must be
    // specified) is the choice of scheduling algorithm.

    char c;
    unsigned long long int i;
    
    for (i = 1; i < argc; i++)
    {
	if (!strcmp(argv[i], "-alg"))
	{
	    i++;
	    if (!strcmp(argv[i], "rr"))
			sps->sched_alg = RR;
	    else if (!strcmp(argv[i], "sjf"))
			sps->sched_alg = SJF;
	    else if (!strcmp(argv[i], "fcfs"))
			sps->sched_alg = FCFS;
	    else
	    {
		usage("Error: invalid scheduling algorithm (-alg).\n");
		return 1;
	    }
	}
	else if (!strcmp(argv[i], "-init_jobs"))
	{
	    i++;
	    if (sscanf(argv[i], "%d%c", &sps->init_jobs, &c) != 1
		|| sps->init_jobs < 0)
	    {
		usage("Error: invalid argument to -init_jobs\n");
		return 1;
	    }
	}
	else if(!strcmp(argv[i], "-total_jobs"))
	{
		i++;
	    if (sscanf(argv[i], "%d", &sps->total_jobs) != 1
		|| sps->total_jobs < 0)
	    {
		usage("Error: invalid argument to -total_jobs\n");
		return 1;
	    }
	}
	else if(!strcmp(argv[i], "-prob_comp_time"))
	{
		i++;
	    if (sscanf(argv[i], "%le", &sps->lambda) != 1
		|| sps->lambda < 0)
	    {
		usage("Error: invalid argument to -prob_comp_time\n");
		return 1;
	    }
	}
	else if(!strcmp(argv[i], "-sched_time"))
	{
		i++;
	    if (sscanf(argv[i], "%d", &sps->sched_time) != 1
		|| sps->sched_time < 0)
	    {
		usage("Error: invalid argument to -sched_time\n");
		return 1;
	    }
	}
	else if(!strcmp(argv[i], "-cs_time"))
	{
		i++;
	    if (sscanf(argv[i], "%d", &sps->cont_swtch_time) != 1
		|| sps->cont_swtch_time < 0)
	    {
		usage("Error: invalid argument to -cs_time\n");
		return 1;
	    }
	}
	else if(!strcmp(argv[i], "-tick_time"))
	{
		i++;
	    if (sscanf(argv[i], "%d", &sps->tick_time) != 1
		|| sps->tick_time < 0)
	    {
		usage("Error: invalid argument to -tick_time\n");
		return 1;
	    }
	}	
	else if(!strcmp(argv[i], "-prob_new_job"))
	{
		i++;
	    if (sscanf(argv[i], "%le", &sps->prob_new_job) != 1
		|| sps->prob_new_job < 0)
	    {
		usage("Error: invalid argument to -prob_new_job\n");
		return 1;
	    }
	}
	else if(!strcmp(argv[i], "-randomize"))
	{
		sps->randomize = true;
		srandom(time(NULL));
	}
	}

    return 0;
}
/*Function prototyping*/
void FCFS_(struct simulation_params sim_params, double *response, double *turnaround, double *wait);
void RR_(struct simulation_params sim_params, double *response, double *turnaround, double *wait);
void SJF_(struct simulation_params sim_params, double *response, double *turnaround, double *wait);
int FCFS_schedule(struct simulation_params sim_params, int index);
int SJF_schedule(struct simulation_params sim_params, int index);
int RR_schedule(struct simulation_params sim_params, int index);
void Calculate_time(struct simulation_params sim_params, double *response, double *turnaround, double *wait);
int rand_exp(double lamda);
int random_chance();

int main(int argc, char * argv[])
{
    progname = argv[0];
    struct simulation_params sim_params = {
	.sched_alg = UNDEFINED,
	.init_jobs = DEFAULT_INIT_JOBS,
	.total_jobs = DEFAULT_TOTAL_JOBS,
	.lambda = DEFAULT_LAMBDA,
	.sched_time = DEFAULT_SCHED_TIME,
	.cont_swtch_time = DEFAULT_CONT_SWTCH_TIME,
	.tick_time = DEFAULT_TICK_TIME,
	.prob_new_job = DEFAULT_PROB_NEW_JOB,
	.randomize = DEFAULT_RANDOMIZE
    };


    double average_response_time = 0.42;
    double average_waiting_tJFime = 0.53;
    double average_turnaround_time = 98.1;


    if (process_args(argc, argv, &sim_params) != 0)
	{
		return EXIT_FAILURE;
	}
    /*wipe array of customers for next run*/   
    array_of_customers = malloc(sizeof(*array_of_customers) * sim_params.total_jobs);
	/*For calling scheduing functions*/
    if (sim_params.sched_alg == RR)
	{
		RR_(sim_params, &average_response_time, &average_turnaround_time, &average_waiting_time);
	}
    else if (sim_params.sched_alg == SJF)
	{
		SJF_(sim_params, &average_response_time, &average_turnaround_time, &average_waiting_time);
	}
    else if (sim_params.sched_alg == FCFS)
	{
        FCFS_(sim_params, &average_response_time, &average_turnaround_time, &average_waiting_time);
	}
    printf("For a simulation using the %s scheduling algorithm\n",
	   alg_names[sim_params.sched_alg]);
    printf("with the following parameters:\n");
    printf("    init jobs           = %d\n", sim_params.init_jobs);
    printf("    total jobs          = %d\n", sim_params.total_jobs);
    printf("    lambda              = %.6f\n", sim_params.lambda);
    printf("    sched time          = %d\n", sim_params.sched_time);
    printf("    context switch time = %d\n", sim_params.cont_swtch_time);
    printf("    tick time           = %d\n", sim_params.tick_time);
    printf("    prob of new job     = %.6f\n", sim_params.prob_new_job);
    printf("    randomize           = %s\n",
	   sim_params.randomize ? "true" : "false");
    printf("the following results were obtained:\n");
    printf("    Average response time:   %10.6lf\n", average_response_time);
    printf("    Average turnaround time: %10.6lf\n", average_turnaround_time);
    printf("    Average waiting time:    %10.6lf\n", average_waiting_time);

    return EXIT_SUCCESS;
}

/*
 * Name:	FCFS_()
 * Purpose:	To perform First Come First Serve scheduing argrithm
 * Arguments:	sim_params, response, turnaround, wait.
 * Outputs:	none.
 * Returns:	none.
 * Bugs:	Numbers do not match valid values given by the prof, Program crashes after many simulations.
 */
void FCFS_(struct simulation_params sim_params, double *response, double *turnaround, double *wait)
{
    
	unsigned long long int index = 0;
    unsigned long long int count = 0;
    /*initalizing inital jobs*/
	while(count < sim_params.init_jobs)
	{
		if(top >= sim_params.total_jobs)
		{
			array_of_customers = (struct customer *)realloc(array_of_customers, sizeof(struct customer) * (sim_params.total_jobs + top));
		}
		/*sets arrival time and burst time of inital jobs*/
		array_of_customers[top].arrival_time = global_time;
    	array_of_customers[top].burst_time = rand_exp(sim_params.lambda);
    	top++;
		count++;
	}
    /*while index is less that total number of mac jobs*/
	while (index < sim_params.total_jobs)
    {
        if (random_chance() < (100 * sim_params.prob_new_job))
        {
			/*Add new job to the queue*/
			if(top >= sim_params.total_jobs)
			{
				array_of_customers = (struct customer *)realloc(array_of_customers, sizeof(struct customer) * (sim_params.total_jobs + top));
			}
			array_of_customers[top].arrival_time = global_time;
    		array_of_customers[top].burst_time = rand_exp(sim_params.lambda);
    		top++;
        }
        /*As long as there is soemthing in the queue, call scheduling function*/
		if (index < top)
        {
            index = FCFS_schedule(sim_params, index);
        }
		global_time = global_time + (1000 * sim_params.tick_time);
    }
    Calculate_time(sim_params, response, turnaround, wait);
    return;
}
/*
 * Name:	RR_()
 * Purpose:	To perform the round robin scheduing argrithm.
 * Arguments:	sim_params, response, turnaround, wait.
 * Outputs:	none.
 * Returns:	none.
 * Bugs:	This does not function properly, causes program crashes.
 * Notes: This is the same as the FCFS_(), except RR_schedule() is called rather than RR_schedule().
 */
void RR_(struct simulation_params sim_params, double *response, double *turnaround, double *wait)
{
    unsigned long long int index = 0;
    unsigned long long int count = 0;
	/*initalizing inital jobs*/
    while(count < sim_params.init_jobs)
	{
		/*sets arrival time and burst time of inital jobs*/
		if(top >= sim_params.total_jobs)
		{
			array_of_customers = (struct customer *)realloc(array_of_customers, sizeof(struct customer) * (sim_params.total_jobs + top));
		}
		array_of_customers[top].arrival_time = global_time;
    	array_of_customers[top].burst_time = rand_exp(sim_params.lambda);
    	top++;
		count++;
	}
    while (index < sim_params.total_jobs)
    {
		/*As long as there is soemthing in the queue, call scheduling function*/
        if (random_chance() < (100 * sim_params.prob_new_job))
        {
			if(top >= sim_params.total_jobs)
			{
				array_of_customers = (struct customer *)realloc(array_of_customers, sizeof(struct customer) * (sim_params.total_jobs + top));
			}
			array_of_customers[top].arrival_time = global_time;
    		array_of_customers[top].burst_time = rand_exp(sim_params.lambda);
    		top++;
        }
        if (index < top)
        {
            index = RR_schedule(sim_params, index);
        }
        global_time = global_time + (1000 * sim_params.tick_time);
    }
    Calculate_time(sim_params, response, turnaround, wait);
    return;
}
/*
 * Name:	SJF_()
 * Purpose:	To perform the shortest job first scheduing argrithm.
 * Arguments:	sim_params, response, turnaround, wait.
 * Outputs:	none.
 * Returns:	none.
 * Bugs:	Numbers do not match valid values given by the prof, Program crashes after many simulations.
 * Notes: This is the same as the FCFS_(), except SJF_schedule() is called rather than RR_schedule().
 */
void SJF_(struct simulation_params sim_params, double *response, double *turnaround, double *wait)
{
	    
	int index = 0;
    int count = 0;
	/*initalizing inital jobs*/
    while(count < sim_params.init_jobs)
	{
		/*sets arrival time and burst time of inital jobs*/
		if(top >= sim_params.total_jobs)
		{
			array_of_customers = (struct customer *)realloc(array_of_customers, sizeof(struct customer) * (sim_params.total_jobs + top));
		}
		array_of_customers[top].arrival_time = global_time;
    	array_of_customers[top].burst_time = rand_exp(sim_params.lambda);
    	top++;
		count++;
	}
    while (index < sim_params.total_jobs)
    {
		/*As long as there is soemthing in the queue, call scheduling function*/
        if (random_chance() < (100 * sim_params.prob_new_job))
        {
			if(top >= sim_params.total_jobs)
			{
				array_of_customers = (struct customer *)realloc(array_of_customers, sizeof(struct customer) * (sim_params.total_jobs + top));
			}
			array_of_customers[top].arrival_time = global_time;
    		array_of_customers[top].burst_time = rand_exp(sim_params.lambda);
    		top++;
        }
        if (index < top)
        {
            index = SJF_schedule(sim_params, index);
        }
        global_time = global_time + (1000 * sim_params.tick_time);
    }
    Calculate_time(sim_params, response, turnaround, wait);
    return;
}
/*
 * Name:	random_chance()
 * Purpose:	return a random number between 1 and 100.
 * Arguments:
 * Outputs:	none.
 * Returns:	rand.
 * Bugs:	none.
 * Notes: Used to determine if a new job is added to the queue 
 */
int random_chance()
{
    unsigned long long int rand = 0.0;
	rand = random() % 100;
    rand++;
    return rand;
}
/*
 * Name:	FCFS_schedule()
 * Purpose:	performs first come first serve scheduling.
 * Arguments: sim_params, index.
 * Outputs:	none.
 * Returns:	index.
 * Bugs:	I'm not sure if this is functioning properly as the values returned do not match values given by the prof. 
 */
int FCFS_schedule(struct simulation_params sim_params, int index)
{

    int finish_time;
	/*finish_time is set to what global time will be when it completes*/
	finish_time = array_of_customers[index].first_serve_time + array_of_customers[index].burst_time;
    /*if job has finihsed*/
	if (finish_time <= global_time)			
    {
        /*move index to next item in queue and set starting values of next item*/
		index++;
		int temp_index = index - 1;
        array_of_customers[temp_index].depart_time = global_time;
    	array_of_customers[index].first_serve_time = global_time;
    	bottom++;
    	global_time = global_time - sim_params.cont_swtch_time;
    }
    global_time = global_time - sim_params.sched_time;
    return index;
}
/*
 * Name:	SJF_schedule()
 * Purpose:	performs shortest job first scheduling
 * Arguments: sim_params, index.
 * Outputs:	none.
 * Returns:	index.
 * Bugs:	I'm not sure if this is functioning properly as the values returned do not match values given by the prof. 
 */
int SJF_schedule(struct simulation_params sim_params, int index)
{
	unsigned long long int count = 0;
    unsigned long long int finish_time;
	/*for all items in queue, checks if total time to complete is smaller that current job */
	while(count <= bottom)
	{
		if((array_of_customers[index].burst_time - array_of_customers[index].first_serve_time) < array_of_customers[count + 1].burst_time - array_of_customers[count].first_serve_time)
		{
			index = count; 
		}
		count++;
	}
	/*finish_time is set to what global time will be when it completes*/
	finish_time = array_of_customers[index].first_serve_time + array_of_customers[index].burst_time;
     /*if job has finihsed*/
	if (finish_time <= global_time)			
    {
        /*move index to next item in queue and set starting values of next item*/
		index++;
		int temp_index = index - 1;
        array_of_customers[temp_index].depart_time = global_time;
    	array_of_customers[index].first_serve_time = global_time;
    	bottom++;
    	global_time = global_time - sim_params.cont_swtch_time;
    }
    global_time = global_time - sim_params.sched_time;
    return index;
}
/*
 * Name:	RR_schedule()
 * Purpose:	performs round robin scheduling algrithm.
 * Arguments: sim_params, index.
 * Outputs:	none.
 * Returns:	index.
 * Bugs:	I'm not sure if this is functioning properly as the values returned do not match values given by the prof. 
 */
int RR_schedule(struct simulation_params sim_params, int index)
{
	unsigned long long int count = 0;
    unsigned long long int finish_time;
	/*This rotates the index from one job to the next*/
	if(index < top)
	{
		index++;
	}
	/*This rotates the index from one job to the next*/
	else if(index == top)
	{
		index = bottom; 
	}
	/*finish_time is set to what global time will be when it completes*/
	finish_time = array_of_customers[index].first_serve_time + array_of_customers[index].burst_time;
    /*if job has finihsed*/
	if (finish_time <= global_time)			
    {
        /*move index to next item in queue and set starting values of next item*/
		index++;
		int temp_index = index - 1;
        array_of_customers[temp_index].depart_time = global_time;
    	array_of_customers[index].first_serve_time = global_time;
    	bottom++;
    	global_time = global_time - sim_params.cont_swtch_time;
    }
    global_time = global_time - sim_params.sched_time;
    return index;
}
/*
 * Name:	Calculate_time()
 * Purpose:	calculates stats for each schedulign algrithm.
 * Arguments: sim_params, response, turnaround, wait.
 * Outputs:	none.
 * Returns:	void.
 * Bugs:	I'm not sure if this is functioning properly as the values returned do not match values given by the prof. 
 */
void Calculate_time(struct simulation_params sim_params, double *response, double *turnaround, double *wait)
{
    double total_turnaround_time = 0;
    double total_response_time = 0;
    double total_wait_time = 0;
    for (int i = 0; i < sim_params.total_jobs; i++)
    {
		/*calculates individual values for each sim*/
        total_response_time = total_response_time + (array_of_customers[i].first_serve_time - array_of_customers[i].arrival_time);
        total_turnaround_time = total_turnaround_time + (array_of_customers[i].depart_time - array_of_customers[i].arrival_time);
        total_wait_time = total_wait_time + (array_of_customers[i].first_serve_time - array_of_customers[i].arrival_time);
    }
    /*calculates average sim values and saves them for printing later*/
	*response = (total_response_time / sim_params.total_jobs) / 1000000;
    *turnaround = (total_turnaround_time / sim_params.total_jobs) / 1000000;
    *wait = (total_wait_time / sim_params.total_jobs) / 1000000;
    return;
}
/*
 * Name:	rand_exp()
 * Purpose:	provide a random int
 * Arguments: lamda.
 * Outputs:	none.
 * Returns:	round.
 * Bugs:	I'm not sure if this is functioning properly as the values returned do not match values given by the prof. 
 * Notes: Provided by the prof, modified to return an int rather than a double. 
 */
int rand_exp(double lamda)
{
    int64_t divisor = (__int64_t)RAND_MAX + 1;
    double u_0_to_almost_1;
    double raw_value;

    u_0_to_almost_1 = (double)random() / divisor;
    raw_value = log(1 - u_0_to_almost_1) / -lamda;
    return round(raw_value * 1000000);
}