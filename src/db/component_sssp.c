#include "graph.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>

/* Place the code for your Dijkstra implementation in this file */

//HELPER FUNCTIONS:

int get_vert_number(component_t c){ 
	//helper function to get the number of vertices in the component
	
	
	int bytes;
	size_t size, len;
	off_t off;
	
	assert(c->sv != NULL);
	
	size = schema_size(c->sv);
	bytes = sizeof(vertexid_t) + size;
	
	char* buf = malloc(bytes);
	
	int counter = 0;
	
	for (off = 0;;) { //iterate through the vertices and count each of them
		lseek(c->vfd, off, SEEK_SET);
		len = read(c->vfd, buf, bytes);
		if (len <= 0) break;
		
		off =+ bytes;
		counter++;
		}
	
	free(buf);
	return counter;
	}

int get_weight(component_t c, vertexid_t a, vertexid_t b, char* name){
	//helper function to grab an edge's weight.
	struct edge e;
	edge_t e1;
	
	edge_init(&e);
    edge_set_vertices(&e, a, b);
    e1 = component_find_edge_by_ids(c, &e);
    if (e1 == NULL) {
        return INT_MAX;
    }
    int off = tuple_get_offset(e1->tuple, name);
    int wt = tuple_get_int(e1->tuple->buf + off);
    return wt;
	
	}


int
component_sssp(
        component_t c,
        vertexid_t v1,
        vertexid_t v2,
        int *n,
        int *total_weight,
        vertexid_t **path)
{
	
	/*
	 * Figure out which attribute in the component edges schema you will
	 * use for your weight function
	 */
	
	//locate integer attribute name.
	
	attribute_t weightAttr;
	attribute_t attr;
	for (attr = c->se->attrlist; attr->next != NULL; attr = attr->next){
		if (attr->bt == 4){
			weightAttr = attr;
			break;
			}
		}
	//if such an attribute was not found, return as error.
	if (weightAttr->bt != 4){ 
		printf("ERROR: Valid weight attribute not found in component edge schema.");
		return (-1);}
	
	
	/*
	 * Execute Dijkstra on the attribute you found for the specified
	 * component
	 */
	//get an array of all the vertices. 
	int vmax = get_vert_number(c);
	vertexid_t verts[vmax]; 
	struct vertex *i;
	
	int count=0;
	
	int v1_exists = 0;
	int v2_exists = 0;
	int v1_index = -1;
	
	
	//put all vertices in the array, also checks if v1 and v2 are in the set.
	for (i = c->v; i != NULL; i=i->next){
		verts[count] = i->id;
		
		if (i->id == v1){
			v1_exists = 1;
			v1_index = count;
			}
		
		if (i->id == v2){
			v2_exists = 1;
			}
		
		count++;
		}
	//initialize distance, cost, visited, and parent lists, since I have no time to really optimize anything, the previous list will be the size of the longest possible path given arbitrary vertices, or going through every vertex in the graph.
	int dist[vmax];
	int visited[vmax];
	int costs[vmax];
	vertexid_t parents[vmax];
	
	int j;
	for (j=0; j < vmax; j++){
		costs[j] = INT_MAX;
		dist[j] = INT_MAX;
		visited[j] = 0;
		parents[j] =-1;
		}

	visited[v1_index] = 1;
	costs[v1_index] = 0;
	dist[v1_index] = 0;
	
	//check if v1 and v2 are in this set
	if (!(v1_exists && v2_exists)){
		printf("ERROR: Source and Destination vertices not found in component.");
		return (-1);
		}
		
	int valid_path;
	
	
	//get costs of v1
	valid_path=0;
	
	for(j=0; j < vmax; j++){
		int weight = get_weight(c,v1,verts[j],weightAttr->name);
		if (weight != INT_MAX){
			costs[j] = weight;
			parents[j] = v1;
			valid_path=1;
			}
		} 
	if (!valid_path){
		printf("ERROR: A path between those vertices does not exist...");
		return(-1);
		}
		
	//Main Dijkstra loop
	int done = 0; //boolean for the while loop;
	int myindex;
	while(!done){
		//get the minimum cost of the unvisited nodes.
		int min = INT_MAX;
		for (j=0; j < vmax; j++){
			if (visited[j] == 0 && costs[j] < min){
				min = costs[j];
				myindex = j;
				}
			
			}
		dist[myindex] = costs[myindex];
		
		valid_path=0;
	
		for(j=0; j < vmax; j++){
			int weight = get_weight(c,verts[myindex],verts[j],weightAttr->name);
			if (weight != INT_MAX){
				int maxweight = dist[myindex] + weight;
				if (maxweight < costs[j] && visited[j] == 0){
					costs[j] = maxweight;
					parents[j] = verts[myindex];}
				
				valid_path=1;
				}
			} 
		if (!valid_path){
			printf("ERROR: A path between those vertices does not exist...");
			return(-1);
			}
		//check if we're done
		done = 1;
		for(j = 0; j < vmax; j++){
			if (visited[j] == 0) {
				done = 0;
				break;
				}
			}
		
		}
	//finishing up: Build and print the path and display cost and total
	//weight
	int nodecount = 0;
	vertexid_t current = v2;
	vertexid_t pathbuilder[vmax];
	
	while (current != -1){
		pathbuilder[nodecount] = current;
		nodecount++; 
		int nextnode = -1;
		for (j=0; j < vmax; j++){
			if (verts[j] == current){
				nextnode = j;
				break;}
			}
		 current = parents[nextnode];
		} 
	
	printf("Path: ");
	for (j=nodecount-1; j >= 0; j--){
		
		if (j>0) printf("%llu ->", pathbuilder[j]); else printf("%llu\n", pathbuilder[j]);
		
		}
	
	
	*n = nodecount; 
	*total_weight = dist[myindex];

	/* Change this as needed */
	return (0);
}
