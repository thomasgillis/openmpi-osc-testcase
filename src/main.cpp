#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "mpi.h"

#ifdef ACTIVE
#define COMM_ACTIVE
#endif

#define N 100
#define SIZE 1000000

int main(int argc, char** argv){
    MPI_Init(&argc,&argv);

    int rank, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
    
    // allocate and init two arrays, one for me and one that I will take from my neighbor
    // two arrays of 100x100x100
    double* array_me  =(double*) malloc(SIZE*sizeof(double));
    double* array_ngh =(double*) malloc(SIZE*sizeof(double));

    for(int iz=0; iz<N; ++iz){
        for(int iy=0; iy< N; ++iy){
            for(int ix=0; ix< N; ++ix){
                array_me [ix+N*(iy+N*iz)] = rank*M_PI;
                array_ngh[ix+N*(iy+N*iz)] = 0.0;
            }
        }
    }

    // create the window
    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Win my_win;
    MPI_Win_create(array_me,SIZE*sizeof(double),sizeof(double),info,MPI_COMM_WORLD,&my_win);
    MPI_Info_free(&info);

    // create the datatype for a 50x50x50 subdomain
    MPI_Aint stride_x,trash_lb;
    MPI_Type_get_extent(MPI_DOUBLE,&trash_lb,&stride_x);

    // in x direction
    MPI_Datatype type_x, type_xy, type_xyz;
    MPI_Type_create_hvector(50, 1, stride_x, MPI_DOUBLE, &type_x);
    MPI_Type_create_hvector(50, 1, N*stride_x, type_x, &type_xy);
    MPI_Type_create_hvector(50, 1, N*N*stride_x, type_xy, &type_xyz);

    MPI_Type_commit(&type_xyz);
    MPI_Type_free(&type_x);
    MPI_Type_free(&type_xy);

    // create a group with the cpu which is +1 wrt me
    int trg_rank  = (comm_size+rank+1)%comm_size; // I take from my neighbor on the right 
    int orig_rank = (comm_size+rank-1)%comm_size; // I receive from my neighbor on the left

    MPI_Group win_group, send_group;
    MPI_Win_get_group(my_win,&win_group);
    
    MPI_Group trg_group; // my target group
    MPI_Group_incl(win_group,1,&trg_rank,&trg_group);

    MPI_Group orig_group; // my origin group
    MPI_Group_incl(win_group,1,&orig_rank,&orig_group);

    const int n_repeat = 500;

    double t0 = MPI_Wtime();

#ifndef COMM_ACTIVE
    MPI_Win_fence(0,my_win);
#endif

    for(int ir=0; ir<n_repeat; ++ir){
#ifdef COMM_ACTIVE
        // start the exposures
        MPI_Win_post(orig_group,MPI_MODE_NOPUT,my_win);

        // strat my own calls
        MPI_Win_start(trg_group,0,my_win);
#else
        MPI_Win_lock(MPI_LOCK_SHARED,trg_rank,0,my_win);
#endif

        // send the get
        MPI_Get(array_ngh,SIZE,MPI_DOUBLE,trg_rank,0,SIZE,MPI_DOUBLE,my_win);

#ifdef COMM_ACTIVE
        // end my own calls
        MPI_Win_complete(my_win);
        // end the exposures
        MPI_Win_wait(my_win);
#else
        MPI_Win_unlock(trg_rank,my_win);
#endif
    }        
    // take the final timing
    double t1 = MPI_Wtime();
    double t = (t1-t0)/n_repeat;

    // average the time over the cpus
    double t_avg, t_min, t_max;
    MPI_Allreduce(&t,&t_avg,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
    MPI_Allreduce(&t,&t_min,1,MPI_DOUBLE,MPI_MIN,MPI_COMM_WORLD);
    MPI_Allreduce(&t,&t_max,1,MPI_DOUBLE,MPI_MAX,MPI_COMM_WORLD);
    t_avg = t_avg/comm_size;

    if(rank == 0){
#ifdef COMM_ACTIVE
        printf("ACTIVE - measure %d times: %f sec on avg (min = %f, max = %f)\n",n_repeat,t_avg,t_min,t_max);
#else
        printf("PASSIVE - measure %d times: %f sec on avg (min = %f, max = %f)\n",n_repeat,t_avg,t_min,t_max);
#endif
    }
    
    // delete the groups
    MPI_Group_free(&orig_group);
    MPI_Group_free(&trg_group);

    // delete the type
    MPI_Type_free(&type_xyz);

    // delete the arrays
    free(array_me);
    free(array_ngh);

    // free the remaining type
    MPI_Win_free(&my_win);
    MPI_Finalize();
}
