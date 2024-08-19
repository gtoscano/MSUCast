/* Nond-domination based selection routines */
#include <iostream>
#include <vector>
# include <stdio.h>
# include <stdlib.h>
# include <math.h>

#include <fmt/core.h>

# include "global.h"

std::vector<int> selected_pop_indices2;

/* Routine to perform non-dominated sorting */
void fill_nondominated_sort (population *mixed_pop, population *new_pop)
{
    int flag;
    int i, j;
    int end;
    int front_size;
    int archieve_size;
    int rank=1;
    list *pool;
    list *elite;
    list *temp1, *temp2;
    pool = (list *)malloc(sizeof(list));
    elite = (list *)malloc(sizeof(list));
    front_size = 0;
    archieve_size=0;
    pool->index = -1;
    pool->parent = NULL;
    pool->child = NULL;
    elite->index = -1;
    elite->parent = NULL;
    elite->child = NULL;
    temp1 = pool;
    selected_pop_indices2.clear();
    for (i=0; i<2*popsize; i++)
    {
        insert (temp1,i);
        temp1 = temp1->child;
    }
    i=0;
    do
    {
        temp1 = pool->child;
        insert (elite, temp1->index);
        front_size = 1;
        temp2 = elite->child;
        temp1 = del (temp1);
        temp1 = temp1->child;
        do
        {
            temp2 = elite->child;
            if (temp1==NULL)
            {
                break;
            }
            do
            {
                end = 0;
                flag = check_dominance (&(mixed_pop->ind[temp1->index]), &(mixed_pop->ind[temp2->index]));
                if (flag == 1)
                {
                    insert (pool, temp2->index);
                    temp2 = del (temp2);
                    front_size--;
                    temp2 = temp2->child;
                }
                if (flag == 0)
                {
                    temp2 = temp2->child;
                }
                if (flag == -1)
                {
                    end = 1;
                }
            }
            while (end!=1 && temp2!=NULL);
            if (flag == 0 || flag == 1)
            {
                insert (elite, temp1->index);
                front_size++;
                temp1 = del (temp1);
            }
            temp1 = temp1->child;
        }
        while (temp1 != NULL);
        temp2 = elite->child;
        j=i;
        if ( (archieve_size+front_size) <= popsize)
        {
            do
            {
             selected_pop_indices2.push_back(temp2->index);
                copy_ind (&mixed_pop->ind[temp2->index], &new_pop->ind[i]);
                new_pop->ind[i].rank = rank;
                archieve_size+=1;
                temp2 = temp2->child;
                i+=1;
            }
            while (temp2 != NULL);
            assign_crowding_distance_indices (new_pop, j, i-1);
            rank+=1;
        }
        else
        {
            crowding_fill (mixed_pop, new_pop, i, front_size, elite);
            archieve_size = popsize;
            for (j=i; j<popsize; j++)
            {
                new_pop->ind[j].rank = rank;
            }
        }
        temp2 = elite->child;
        do
        {
            temp2 = del (temp2);
            temp2 = temp2->child;
        }
        while (elite->child !=NULL);
    }
    while (archieve_size < popsize);
    while (pool!=NULL)
    {
        temp1 = pool;
        pool = pool->child;
        free (temp1);
    }
    while (elite!=NULL)
    {
        temp1 = elite;
        elite = elite->child;
        free (temp1);
    }

  for (i = 0; i < popsize; i++) {
    auto selected_i =  selected_pop_indices2[i];
    auto dst_uuid_idx = fmt::format("{}_{}", curr_gen, i);
    if (selected_i < popsize) {
      // Parent population.
      auto src_uuid_idx = fmt::format("{}_{}", curr_gen-1, selected_i);
      uuid_surviving_pop_registry[dst_uuid_idx]  = uuid_surviving_pop_registry[src_uuid_idx];
    }
    else {
      auto src_uuid_idx = fmt::format("{}_{}", curr_gen, selected_i-popsize);
      uuid_surviving_pop_registry[dst_uuid_idx]  = uuid_registry[src_uuid_idx];
    }
  }
    return;
}

/* Routine to fill a population with individuals in the decreasing order of crowding distance */
void crowding_fill (population *mixed_pop, population *new_pop, int count, int front_size, list *elite)
{
    int *dist;
    list *temp;
    int i, j;
    assign_crowding_distance_list (mixed_pop, elite->child, front_size);
    dist = (int *)malloc(front_size*sizeof(int));
    temp = elite->child;
    for (j=0; j<front_size; j++)
    {
        dist[j] = temp->index;
        temp = temp->child;
    }
    quicksort_dist (mixed_pop, dist, front_size);
    for (i=count, j=front_size-1; i<popsize; i++, j--)
    {
        copy_ind(&mixed_pop->ind[dist[j]], &new_pop->ind[i]);
        selected_pop_indices2.push_back(dist[j]);
    }
    free (dist);
    return;
}
